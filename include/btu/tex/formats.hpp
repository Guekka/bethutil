#pragma once

#include <btu/tex/dxtex.hpp>

#include <optional>
#include <string>

namespace btu::tex {
class Texture;

struct BestFormatFor
{
    DXGI_FORMAT uncompressed               = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT uncompressed_without_alpha = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT compressed                 = DXGI_FORMAT_BC7_UNORM;
    DXGI_FORMAT compressed_without_alpha   = DXGI_FORMAT_BC1_UNORM;
};

enum class TextureType
{
    Diffuse,
    Normal,
    Cube,
    Specular,
    Glow,
    Parallax,
    ModelSpaceNormal,
    Backlight,
    Skin,
    EnvironmentMask
};

enum class AllowCompressed
{
    Yes,
    No,
};

/// Based on https://forums.nexusmods.com/index.php?/topic/476227-skyrim-nif-files-with-underscores/
auto guess_texture_type(std::u8string_view path) noexcept -> std::optional<TextureType>;

auto guess_best_format(const Texture &tex,
                       BestFormatFor formats,
                       AllowCompressed allow_compressed = AllowCompressed::Yes) noexcept -> DXGI_FORMAT;
} // namespace btu::tex
