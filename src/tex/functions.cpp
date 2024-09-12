#include <btu/tex/compression_device.hpp>
#include <btu/tex/dxtex.hpp>
#include <btu/tex/error_code.hpp>
#include <btu/tex/functions.hpp>

#include <algorithm>

namespace btu::tex {
auto decompress(Texture &&file) -> Result
{
    const auto &tex    = file.get();
    const auto &img    = tex.GetImages();
    const size_t &nimg = tex.GetImageCount();
    const auto &info   = tex.GetMetadata();

    ScratchImage timage;
    const auto hr = Decompress(img, nimg, info, DXGI_FORMAT_UNKNOWN /* picks good default */, timage);
    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    file.set(std::move(timage));
    return std::move(file);
}

auto make_transparent_alpha(Texture &&file) -> Result
{
    const auto &tex = file.get();

    if (!DirectX::HasAlpha(tex.GetMetadata().format))
        return tl::make_unexpected(Error(TextureErr::BadInput));

    constexpr auto transform = [](DirectX::XMVECTOR *out_pixels,
                                  const DirectX::XMVECTOR *in_pixels,
                                  const size_t width,
                                  [[maybe_unused]] size_t) {
        const auto transparent = DirectX::XMVectorSet(0, 0, 0, 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const auto *end = in_pixels + width;
        std::transform(in_pixels, end, out_pixels, [&](auto pix) {
            return XMVectorSelect(transparent, pix, DirectX::g_XMSelect1110);
        });
    };

    ScratchImage timage;
    const auto hr = TransformImage(tex.GetImages(), tex.GetImageCount(), tex.GetMetadata(), transform, timage);

    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    file.set(std::move(timage));
    return std::move(file);
}

static auto convert_uncompressed(const ScratchImage &image,
                                 ScratchImage &timage,
                                 DXGI_FORMAT format,
                                 [[maybe_unused]] CompressionDevice &dummy) -> HRESULT
{
    const auto *const img = image.GetImages();
    if (img == nullptr)
        return E_INVALIDARG;
    const size_t nimg = image.GetImageCount();

    return Convert(img,
                   nimg,
                   image.GetMetadata(),
                   format,
                   DirectX::TEX_FILTER_SEPARATE_ALPHA,
                   DirectX::TEX_THRESHOLD_DEFAULT,
                   timage);
}

// The reason for this to be separated in another file is that both libraries define DXGI_FORMAT
// So there's a conflict
// Also, we use another library because DirectXTex BC7 CPU encoder is unbearably slow
auto convert_bc7(const uint8_t *source, uint8_t *dest, uint32_t width, uint32_t height, size_t slice_pitch)
    -> tl::expected<void, Error>;

static auto convert_compressed(const ScratchImage &image,
                               ScratchImage &timage,
                               DXGI_FORMAT format,
                               [[maybe_unused]] CompressionDevice &dev) -> HRESULT
{
    const auto *const img = image.GetImages();
    if (img == nullptr)
        return E_INVALIDARG;
    const size_t nimg = image.GetImageCount();

    const bool bc6hbc7 = [&]() noexcept {
        switch (format)
        {
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB: return true;
            default: return false;
        }
    }();

    if (bc6hbc7)
    {
        // hardware impl, only works on d3d11
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        HRESULT bc7_ret    = E_FAIL;
        const bool applied = dev.try_apply([&](auto *d3d11_dev) {
            bc7_ret = Compress(d3d11_dev,
                               img,
                               nimg,
                               image.GetMetadata(),
                               format,
                               DirectX::TEX_COMPRESS_DEFAULT,
                               DirectX::TEX_THRESHOLD_DEFAULT,
                               timage);
        });

        if (applied)
            return bc7_ret;
#endif

        // slower bc7 impl, but faster than dxtex
        auto metadata   = image.GetMetadata();
        metadata.format = DXGI_FORMAT_BC7_UNORM;
        const auto hr   = timage.Initialize(metadata);
        if (FAILED(hr))
            return hr;

        for (size_t i = 0; i < image.GetImageCount(); ++i)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const auto &simg = image.GetImages()[i];
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const auto &timg = timage.GetImages()[i];

            const auto res = convert_bc7(simg.pixels,
                                         timg.pixels,
                                         static_cast<uint32_t>(simg.width),
                                         static_cast<uint32_t>(simg.height),
                                         simg.slicePitch);
            if (!res)
                return E_FAIL;
        }
        return S_OK;
    }

    constexpr auto compress_flags = [] {
#if _OPENMP
        return DirectX::TEX_COMPRESS_PARALLEL;
#else
        return DirectX::TEX_COMPRESS_DEFAULT;
#endif
    }();

    return Compress(img,
                    nimg,
                    image.GetMetadata(),
                    format,
                    compress_flags,
                    DirectX::TEX_THRESHOLD_DEFAULT,
                    timage);
}

auto convert(Texture &&file, DXGI_FORMAT format, CompressionDevice &dev) -> Result
{
    const auto &tex = file.get();
    const auto info = tex.GetMetadata();

    const bool uncompressed_required = info.width < 4 || info.height < 4;
    //Textures smaller than that cannot be compressed
    if (uncompressed_required && DirectX::IsCompressed(info.format))
        return tl::make_unexpected(Error(TextureErr::BadInput));

    ScratchImage timage;

    const auto f = DirectX::IsCompressed(format) ? convert_compressed : convert_uncompressed;

    if (const auto hr = f(tex, timage, format, dev); FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    file.set(std::move(timage));
    return std::move(file);
}

static auto prepare_generate_mipmaps(Texture &&file) -> Result
{
    const auto &tex = file.get();
    // Mips generation only works on a single base image, so strip off existing mip levels
    const auto &info = tex.GetMetadata();
    ScratchImage timage;

    TexMetadata mdata = info;
    mdata.mipLevels   = 1;
    if (const auto hr = timage.Initialize(mdata); FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    for (size_t i = 0; i < info.arraySize; ++i)
    {
        const auto hr = CopyRectangle(*tex.GetImage(0, i, 0),
                                      DirectX::Rect(0, 0, info.width, info.height),
                                      *timage.GetImage(0, i, 0),
                                      DirectX::TEX_FILTER_SEPARATE_ALPHA,
                                      0,
                                      0);
        if (FAILED(hr))
            return tl::make_unexpected(error_from_hresult(hr));
    }

    file.set(std::move(timage));
    return std::move(file);
}

static auto generate_mipmaps_impl(Texture &&file) -> Result
{
    const auto &tex   = file.get();
    const auto &info  = tex.GetMetadata();
    const size_t mips = optimal_mip_count({.w = info.width, .h = info.height});

    ScratchImage timage;
    const auto hr = GenerateMipMaps(tex.GetImages(),
                                    tex.GetImageCount(),
                                    tex.GetMetadata(),
                                    DirectX::TEX_FILTER_SEPARATE_ALPHA,
                                    mips,
                                    timage);

    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    file.set(std::move(timage));
    return std::move(file);
}

auto generate_mipmaps(Texture &&file) -> Result
{
    return prepare_generate_mipmaps(std::move(file)).and_then(generate_mipmaps_impl);
}

auto resize(Texture &&file, Dimension dim) -> Result
{
    const auto &tex  = file.get();
    const auto &info = tex.GetMetadata();

    ScratchImage timage;

    // DirectX::Resize is dumb. If WIC is used, it will convert the image to
    // R32G32B32A32 It works for small images... But will, for example, allocate
    // 1gb for a 8k picture. So disable WIC
    constexpr auto filter = DirectX::TEX_FILTER_SEPARATE_ALPHA | DirectX::TEX_FILTER_FORCE_NON_WIC;
    const auto hr         = Resize(tex.GetImages(), tex.GetImageCount(), info, dim.w, dim.h, filter, timage);
    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    file.set(std::move(timage));
    return std::move(file);
}
} // namespace btu::tex
