#pragma once

#include "btu/tex/detail/common.hpp"
#include "btu/tex/detail/formats.hpp"
#include "btu/tex/dimension.hpp"

namespace DirectX { // NOLINT
auto operator==(const ScratchImage &lhs, const ScratchImage &rhs) noexcept -> bool;
auto operator==(const TexMetadata &lhs, const TexMetadata &rhs) noexcept -> bool;
} // namespace DirectX

namespace btu::tex {
class CompressionDevice;

[[nodiscard]] auto decompress(const ScratchImage &tex) -> Result;
[[nodiscard]] auto make_opaque_alpha(const ScratchImage &tex) -> Result;
[[nodiscard]] auto convert(const ScratchImage &tex, DXGI_FORMAT format, CompressionDevice &dev) -> Result;

[[nodiscard]] constexpr auto optimal_mip_count(Dimension dim) noexcept -> size_t
{
    size_t mips = 1;
    auto size   = std::max(dim.w, dim.h);

    while (size > 1)
    {
        size /= 2;
        ++mips;
    }
    return mips;
}

[[nodiscard]] auto generate_mipmaps(const ScratchImage &tex) -> Result;
[[nodiscard]] auto resize(const ScratchImage &tex, Dimension dim) -> Result;
} // namespace btu::tex
