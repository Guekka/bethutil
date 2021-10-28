#pragma once

#include "btu/tex/detail/common.hpp"
#include "btu/tex/detail/formats.hpp"

#include <variant>

namespace DirectX { // NOLINT
auto operator==(const ScratchImage &lhs, const ScratchImage &rhs) noexcept -> bool;
auto operator==(const TexMetadata &lhs, const TexMetadata &rhs) noexcept -> bool;
} // namespace DirectX

namespace btu::tex {
class CompressionDevice;

[[nodiscard]] auto decompress(const ScratchImage &tex) -> Result;
[[nodiscard]] auto make_opaque_alpha(const ScratchImage &tex) -> Result;
[[nodiscard]] auto convert(const ScratchImage &tex, DXGI_FORMAT format, CompressionDevice &dev) -> Result;

[[nodiscard]] auto optimal_mip_count(size_t width, size_t height) noexcept -> size_t;
[[nodiscard]] auto generate_mipmaps(const ScratchImage &tex) -> Result;

struct TextureResizeArg
{
    struct Ratio
    {
        uint8_t ratio;

        size_t min_width;
        size_t min_height;
    };

    struct Absolute
    {
        size_t width;
        size_t height;
    };

    std::variant<Ratio, Absolute> data;
};

[[nodiscard]] auto compute_resize_dimension(const DirectX::TexMetadata &info, const TextureResizeArg &args)
    -> std::pair<size_t, size_t>;

[[nodiscard]] auto resize(const ScratchImage &tex, size_t x, size_t y) -> Result;
} // namespace btu::tex
