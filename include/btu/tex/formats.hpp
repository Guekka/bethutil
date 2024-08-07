#pragma once

#include <btu/common/json.hpp>
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

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    BestFormatFor, uncompressed, uncompressed_without_alpha, compressed, compressed_without_alpha);

enum class TextureType : std::uint8_t
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

NLOHMANN_JSON_SERIALIZE_ENUM(TextureType,
                             {{TextureType::Diffuse, "diffuse"},
                              {TextureType::Normal, "normal"},
                              {TextureType::Cube, "cube"},
                              {TextureType::Specular, "specular"},
                              {TextureType::Glow, "glow"},
                              {TextureType::Parallax, "parallax"},
                              {TextureType::ModelSpaceNormal, "model_space_normal"},
                              {TextureType::Backlight, "backlight"},
                              {TextureType::Skin, "skin"},
                              {TextureType::EnvironmentMask, "environment_mask"}});

struct GuessBestFormatArgs
{
    bool opaque_alpha     = false;
    bool allow_compressed = true;
    bool force_alpha      = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GuessBestFormatArgs, opaque_alpha, allow_compressed, force_alpha);

/// Based on https://forums.nexusmods.com/index.php?/topic/476227-skyrim-nif-files-with-underscores/
auto guess_texture_type(std::u8string_view path) noexcept -> std::optional<TextureType>;

auto guess_best_format(DXGI_FORMAT current_format,
                       BestFormatFor formats,
                       const GuessBestFormatArgs &guess_params) noexcept -> DXGI_FORMAT;
} // namespace btu::tex
