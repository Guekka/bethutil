#pragma once

#include <dxgiformat.h>

#include <optional>
#include <string>

namespace btu::tex {
class Texture;

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
    Backlight,
    Skin,
    EnvironmentMask
};

/// Based on https://forums.nexusmods.com/index.php?/topic/476227-skyrim-nif-files-with-underscores/
auto guess_texture_type(std::u8string_view path) noexcept -> std::optional<TextureType>;

auto guess_best_format(const Texture &tex, std::optional<TextureType> type = std::nullopt) noexcept
    -> DXGI_FORMAT;
} // namespace btu::tex
