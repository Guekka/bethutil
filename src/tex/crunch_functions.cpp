#include <btu/tex/crunch_functions.hpp>
#include <btu/tex/formats.hpp>
#include <crunch/crnlib.h>

#include <algorithm>
#include <thread>

namespace btu::tex {
using crnlib::dxt_image;
using crnlib::pixel_format;

void set_gamma_correction(mipmapped_texture::resample_params &params, const TextureType tex_type)
{
    // Diffuse textures are assumed to be sRGB.
    if (tex_type == TextureType::Diffuse)
    {
        params.m_gamma = 2.2f;
        params.m_srgb  = true;
    }
    else
    {
        params.m_gamma = 1.0f;
        params.m_srgb  = false;
    }
}

auto resize(CrunchTexture &&file, Dimension dim) -> ResultCrunch
{
    // Resizes the input texture. If compressed, automatically decompresses it. Removes mipmaps.
    mipmapped_texture::resample_params res_params;
    res_params.m_filter_scale  = 1.0f;
    res_params.m_multithreaded = true;

    set_gamma_correction(res_params, file.get_texture_type());

    const auto success = file.get().resize(static_cast<crnlib::uint>(dim.w),
                                           static_cast<crnlib::uint>(dim.h),
                                           res_params);
    if (!success)
    {
        // No error information is propagated from Crunch.
        return tl::make_unexpected(error_from_hresult(E_UNEXPECTED));
    }

    return std::move(file);
}

auto generate_mipmaps(CrunchTexture &&file) -> ResultCrunch
{
    mipmapped_texture::generate_mipmap_params gen_params;
    gen_params.m_multithreaded = true;
    gen_params.m_max_mips      = crn_limits::cCRNMaxLevels;
    gen_params.m_min_mip_size  = 1;

    set_gamma_correction(gen_params, file.get_texture_type());

    const auto success = file.get().generate_mipmaps(gen_params, true);
    if (!success)
    {
        // No error information is propagated from Crunch.
        return tl::make_unexpected(error_from_hresult(E_UNEXPECTED));
    }

    return std::move(file);
}

auto convert(CrunchTexture &&file, DXGI_FORMAT format) -> ResultCrunch
{
    pixel_format crunch_format;
    switch (format)
    {
        case DXGI_FORMAT_BC1_UNORM: crunch_format = pixel_format::PIXEL_FMT_DXT1; break;
        case DXGI_FORMAT_BC3_UNORM: crunch_format = pixel_format::PIXEL_FMT_DXT5; break;
        case DXGI_FORMAT_B8G8R8X8_UNORM: crunch_format = pixel_format::PIXEL_FMT_R8G8B8; break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM: crunch_format = pixel_format::PIXEL_FMT_A8R8G8B8; break;
        default: return tl::make_unexpected(error_from_hresult(ERROR_BAD_FORMAT));
    }

    // Default compression arguments.
    dxt_image::pack_params pack_params;

    // Crunch is capped to 16 threads total, but it should be handled internally.
    const auto threads               = std::thread::hardware_concurrency();
    pack_params.m_num_helper_threads = (threads ? threads : 1) - 1;

    // Disable endpoint caching for best and deterministic results. Time savings are nearly non-existent.
    pack_params.m_endpoint_caching = false;

    pack_params.m_perceptual = file.get_texture_type() == TextureType::Diffuse;

    const auto success = file.get().convert(crunch_format, pack_params);
    if (!success)
    {
        // No error information is propagated from Crunch.
        return tl::make_unexpected(error_from_hresult(E_UNEXPECTED));
    }

    return std::move(file);
}
} // namespace btu::tex
