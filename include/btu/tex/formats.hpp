#pragma once

#include <optional>
#include <string>

namespace btu::tex {
enum class TextureType
{
    Diffuse,
    Normal,
    Cube,
    Bump,
    Specular,
    Glow,
    Parallax,
    ModelSpaceNormal,
    Blur,
    Skin,
    EnvironmentMask
};

/// Based on https://forums.nexusmods.com/index.php?/topic/476227-skyrim-nif-files-with-underscores/
auto guess_texture_type(std::u8string_view path) -> std::optional<TextureType>;
} // namespace btu::tex
