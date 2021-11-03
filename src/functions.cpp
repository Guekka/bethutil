#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOMCX
#define NOSERVICE
#define NOHELP

#include "btu/tex/functions.hpp"

#include "btu/tex/compression_device.hpp"
#include "btu/tex/error_code.hpp"

#include <DirectXTex.h>
#include <btu/common/metaprogramming.hpp>

#include <algorithm>

namespace DirectX {
auto operator==(const TexMetadata &lhs, const TexMetadata &rhs) noexcept -> bool
{
    auto cmp = [&](auto... ptrs) { return ((std::invoke(ptrs, lhs) == std::invoke(ptrs, rhs)) && ...); };
    return cmp(&TexMetadata::arraySize,
               &TexMetadata::depth,
               &TexMetadata::format,
               &TexMetadata::height,
               &TexMetadata::mipLevels,
               &TexMetadata::miscFlags,
               &TexMetadata::miscFlags2,
               &TexMetadata::width);
}

auto operator==(const ScratchImage &lhs, const ScratchImage &rhs) noexcept -> bool
{
    auto cmp = [&](auto... ptrs) { return ((std::invoke(ptrs, lhs) == std::invoke(ptrs, rhs)) && ...); };
    const bool first = cmp(&ScratchImage::GetMetadata,
                           &ScratchImage::GetImageCount,
                           &ScratchImage::GetPixelsSize);
    if (!first)
        return false;

    const auto end = lhs.GetPixels() + lhs.GetPixelsSize(); // NOLINT
    return std::equal(lhs.GetPixels(), end, rhs.GetPixels());
}
} // namespace DirectX

namespace btu::tex {
auto decompress(const ScratchImage &tex) -> Result
{
    const auto &img    = tex.GetImages();
    const size_t &nimg = tex.GetImageCount();
    const auto &info   = tex.GetMetadata();

    DirectX::ScratchImage timage;
    const auto hr = DirectX::Decompress(img, nimg, info, DXGI_FORMAT_UNKNOWN /* picks good default */, timage);
    if (FAILED(hr))
        return tl::make_unexpected(TextureErr::Unknown);

    return timage;
}

auto make_opaque_alpha(const ScratchImage &tex) -> Result
{
    if (!DirectX::HasAlpha(tex.GetMetadata().format))
        return tl::make_unexpected(TextureErr::BadInput);

    constexpr auto transform = [](DirectX::XMVECTOR *out_pixels,
                                  const DirectX::XMVECTOR *in_pixels,
                                  const size_t width,
                                  [[maybe_unused]] size_t) {
        const auto black = DirectX::XMVectorSet(0, 0, 0, 0);
        const auto end   = in_pixels + width; // NOLINT
        std::transform(in_pixels, end, out_pixels, [&](auto &&pix) {
            return XMVectorSelect(black, pix, DirectX::g_XMSelect1110);
        });
    };

    DirectX::ScratchImage timage;
    const auto hr = DirectX::TransformImage(tex.GetImages(),
                                            tex.GetImageCount(),
                                            tex.GetMetadata(),
                                            transform,
                                            timage);

    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    return timage;
}

auto convert_uncompressed(const ScratchImage &image,
                          ScratchImage &timage,
                          DXGI_FORMAT format,
                          [[maybe_unused]] CompressionDevice &dev) -> HRESULT
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

auto convert_compressed(const ScratchImage &image,
                        ScratchImage &timage,
                        DXGI_FORMAT format,
                        CompressionDevice &dev) -> HRESULT
{
    const auto *const img = image.GetImages();
    if (img == nullptr)
        return E_INVALIDARG;
    const size_t nimg = image.GetImageCount();

    const bool bc6hbc7 = [&]() noexcept {
        switch (format)
        {
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB: return true;
            default: return false;
        }
    }();

    if (bc6hbc7 && dev)
        return DirectX::Compress(dev.get_device(),
                                 img,
                                 nimg,
                                 image.GetMetadata(),
                                 format,
                                 DirectX::TEX_COMPRESS_DEFAULT,
                                 DirectX::TEX_THRESHOLD_DEFAULT,
                                 timage);

    return DirectX::Compress(img,
                             nimg,
                             image.GetMetadata(),
                             format,
                             DirectX::TEX_COMPRESS_DEFAULT,
                             DirectX::TEX_THRESHOLD_DEFAULT,
                             timage);
}

auto convert(const ScratchImage &tex, DXGI_FORMAT format, CompressionDevice &dev) -> Result
{
    const auto info                  = tex.GetMetadata();
    const bool uncompressed_required = info.width < 4 || info.height < 4;
    //Textures smaller than that cannot be compressed
    if (uncompressed_required && DirectX::IsCompressed(info.format))
        return tl::make_unexpected(TextureErr::BadInput);

    DirectX::ScratchImage timage;

    auto f = DirectX::IsCompressed(format) ? convert_compressed : convert_uncompressed;

    if (auto result = f(tex, timage, format, dev); FAILED(result))
        return tl::make_unexpected(error_from_hresult(result));

    return timage;
}


auto prepare_generate_mipmaps(const ScratchImage &tex) -> Result
{
    // Mips generation only works on a single base image, so strip off existing mip levels
    const auto &info = tex.GetMetadata();
    DirectX::ScratchImage timage;

    DirectX::TexMetadata mdata = info;
    mdata.mipLevels            = 1;
    if (auto hr = timage.Initialize(mdata); FAILED(hr))
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
    return timage;
}

auto generate_mipmaps_impl(ScratchImage tex) -> Result
{
    const auto &info  = tex.GetMetadata();
    const size_t mips = optimal_mip_count({info.width, info.height});

    DirectX::ScratchImage timage;
    const auto hr = GenerateMipMaps(tex.GetImages(),
                                    tex.GetImageCount(),
                                    tex.GetMetadata(),
                                    DirectX::TEX_FILTER_SEPARATE_ALPHA,
                                    mips,
                                    timage);

    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));
    return timage;
}

auto generate_mipmaps(const ScratchImage &tex) -> Result
{
    return prepare_generate_mipmaps(tex).and_then(generate_mipmaps_impl);
}

auto resize(const ScratchImage &tex, Dimension dim) -> Result
{
    const auto &info = tex.GetMetadata();

    DirectX::ScratchImage timage;
    const auto &img = tex.GetImages();
    if (img == nullptr)
        return tl::make_unexpected(TextureErr::BadInput);

    constexpr auto filter = DirectX::TEX_FILTER_SEPARATE_ALPHA;
    const HRESULT hr      = DirectX::Resize(img, tex.GetImageCount(), info, dim.w, dim.h, filter, timage);
    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    return timage;
}

} // namespace btu::tex
