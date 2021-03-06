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
#include <fstream>

namespace btu::tex {
auto decompress(Texture &&file) -> Result
{
    const auto &tex    = file.get();
    const auto &img    = tex.GetImages();
    const size_t &nimg = tex.GetImageCount();
    const auto &info   = tex.GetMetadata();

    DirectX::ScratchImage timage;
    const auto hr = DirectX::Decompress(img, nimg, info, DXGI_FORMAT_UNKNOWN /* picks good default */, timage);
    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    file.set(std::move(timage));
    return file;
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
        const auto end         = in_pixels + width; // NOLINT
        std::transform(in_pixels, end, out_pixels, [&](auto &&pix) {
            return XMVectorSelect(transparent, pix, DirectX::g_XMSelect1110);
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

    file.set(std::move(timage));
    return file;
}

auto convert_uncompressed(const ScratchImage &image,
                          ScratchImage &timage,
                          DXGI_FORMAT format,
                          [[maybe_unused]] ID3D11Device *dev) -> HRESULT
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

auto convert_compressed(const ScratchImage &image, ScratchImage &timage, DXGI_FORMAT format, ID3D11Device *dev)
    -> HRESULT
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
        return DirectX::Compress(dev,
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

auto convert(Texture &&file, DXGI_FORMAT format, ID3D11Device *dev) -> Result
{
    const auto &tex = file.get();
    const auto info = tex.GetMetadata();

    const bool uncompressed_required = info.width < 4 || info.height < 4;
    //Textures smaller than that cannot be compressed
    if (uncompressed_required && DirectX::IsCompressed(info.format))
        return tl::make_unexpected(Error(TextureErr::BadInput));

    DirectX::ScratchImage timage;

    auto f = DirectX::IsCompressed(format) ? convert_compressed : convert_uncompressed;

    if (const auto hr = f(tex, timage, format, dev); FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    file.set(std::move(timage));
    return file;
}

auto prepare_generate_mipmaps(Texture &&file) -> Result
{
    const auto &tex = file.get();
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

    file.set(std::move(timage));
    return file;
}

auto generate_mipmaps_impl(Texture &&file) -> Result
{
    const auto &tex   = file.get();
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

    file.set(std::move(timage));
    return file;
}

auto generate_mipmaps(Texture &&file) -> Result
{
    return prepare_generate_mipmaps(std::move(file)).and_then(generate_mipmaps_impl);
}

auto resize(Texture &&file, Dimension dim) -> Result
{
    const auto &tex  = file.get();
    const auto &info = tex.GetMetadata();

    DirectX::ScratchImage timage;

    // DirectX::Resize is dumb. If WIC is used, it will convert the image to
    // R32G32B32A32 It works for small image.. But will, for example, allocate
    // 1gb for a 8k picture. So disable WIC
    const auto filter = DirectX::TEX_FILTER_SEPARATE_ALPHA | DirectX::TEX_FILTER_FORCE_NON_WIC;
    const auto hr = DirectX::Resize(tex.GetImages(), tex.GetImageCount(), info, dim.w, dim.h, filter, timage);
    if (FAILED(hr))
        return tl::make_unexpected(error_from_hresult(hr));

    file.set(std::move(timage));
    return file;
}

} // namespace btu::tex
