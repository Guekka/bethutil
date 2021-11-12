#pragma once

#include "btu/tex/detail/common.hpp"
#include "btu/tex/detail/formats_string.hpp"
#include "btu/tex/dimension.hpp"
#include "btu/tex/texture.hpp"

#include <filesystem>

struct ID3D11Device;
namespace btu::tex {
[[nodiscard]] auto decompress(Texture &&file) -> Result;
[[nodiscard]] auto make_transparent_alpha(Texture &&file) -> Result;

/// dev may be null
/// Thread-safety : this function is thread-safe IF ID3D11Device is not shared among threads[[nodiscard]] auto convert(Texture &&file, DXGI_FORMAT format, ID3D11Device *dev) -> Result;
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
