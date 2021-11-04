#pragma once

#include "btu/tex/detail/common.hpp"
#include "btu/tex/detail/formats.hpp"
#include "btu/tex/dimension.hpp"
#include "btu/tex/texture.hpp"

#include <filesystem>

namespace btu::tex {
class CompressionDevice;

[[nodiscard]] auto decompress(Texture &&file) -> Result;
[[nodiscard]] auto make_opaque_alpha(Texture &&file) -> Result;
[[nodiscard]] auto convert(Texture &&file, DXGI_FORMAT format, CompressionDevice &dev) -> Result;

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

[[nodiscard]] auto generate_mipmaps(Texture &&file) -> Result;
[[nodiscard]] auto resize(Texture &&file, Dimension dim) -> Result;
} // namespace btu::tex
