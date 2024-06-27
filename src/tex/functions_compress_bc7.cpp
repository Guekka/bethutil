#if _OPENMP
#include <omp.h>
#endif
#include <bc7enc/rdo_bc_encoder.h>
#include <bc7enc/utils.h>
#include <btu/tex/error_code.hpp>
#include <tl/expected.hpp>

namespace btu::tex {

auto convert_bc7(const uint8_t *source, uint8_t *dest, uint32_t width, uint32_t height, size_t slice_pitch)
    -> tl::expected<void, Error>
{
    rdo_bc::rdo_bc_params rp;

    rp.m_rdo_max_threads = 1;
#if _OPENMP
    constexpr int min_threads = 128; // no idea why, comes from the original code
    rp.m_rdo_max_threads      = std::min(std::max(1, omp_get_max_threads()), min_threads);
#endif
    rp.m_bc7enc_reduce_entropy = true;

    rdo_bc::rdo_bc_encoder encoder;

    utils::image_u8 image(width, height);
    memcpy(image.get_pixels().data(), source, slice_pitch);

    if (!encoder.init(image, rp))
        return tl::make_unexpected(Error(TextureErr::Unknown));

    if (!encoder.encode())
        return tl::make_unexpected(Error(TextureErr::Unknown));

    const auto *res_blocks = static_cast<const uint8_t *>(encoder.get_blocks());
    slice_pitch            = encoder.get_total_blocks_size_in_bytes();

    std::copy_n(res_blocks, slice_pitch, dest);

    return {};
}
} // namespace btu::tex