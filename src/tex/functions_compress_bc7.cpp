#if _OPENMP
#include <omp.h>
#endif
#include <bc7enc/rdo_bc_encoder.h>
#include <bc7enc/utils.h>
#include <btu/tex/error_code.hpp>
#include <tl/expected.hpp>

namespace btu::tex {

auto convert_bc7(const uint8_t *source, uint8_t *dest, size_t width, size_t height, size_t slicePitch)
    -> tl::expected<void, common::Error>
{
    rdo_bc::rdo_bc_params rp;

    rp.m_rdo_max_threads = 1;
#if _OPENMP
    rp.m_rdo_max_threads = std::min(std::max(1, omp_get_max_threads()), 128);
#endif
    rp.m_bc7enc_reduce_entropy = true;

    rdo_bc::rdo_bc_encoder encoder;

    utils::image_u8 image(width, height);
    const auto &dst = reinterpret_cast<uint8_t *>(image.get_pixels().data());
    std::copy(source, source + slicePitch, dst);

    if (!encoder.init(image, rp))
        return tl::make_unexpected(btu::common::Error(btu::tex::TextureErr::Unknown));

    if (!encoder.encode())
        return tl::make_unexpected(btu::common::Error(btu::tex::TextureErr::Unknown));

    const auto res_blocks = reinterpret_cast<const uint8_t *>(encoder.get_blocks());
    slicePitch            = encoder.get_total_blocks_size_in_bytes();

    std::copy(res_blocks, res_blocks + slicePitch, dest);

    return {};
}
} // namespace btu::tex