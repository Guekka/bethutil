#include "btu/tex/formats.hpp"

#include <btu/common/string.hpp>

namespace btu::tex {

auto guess_texture_type(std::u8string_view path) noexcept -> std::optional<TextureType>
{
    constexpr auto suffix = std::u8string_view(u8".dds");
    if (path.size() < suffix.size() + 2)
        return std::nullopt;

    path.remove_suffix(suffix.size());

    const auto type_pos = path.rfind(u8'_');
    if (type_pos == std::u8string_view::npos)
        return TextureType::Diffuse;

    path.remove_prefix(type_pos + 1); // Remove underscore
    auto type = btu::common::to_lower(path);

    if (type == u8"n")
        return TextureType::Normal;
    if (type == u8"s")
        return TextureType::Specular;
    if (type == u8"d")
        return TextureType::Diffuse;
    if (type == u8"g")
        return TextureType::Glow;
    if (type == u8"p")
        return TextureType::Parallax;
    if (type == u8"e")
        return TextureType::Cube;
    if (type == u8"msn")
        return TextureType::ModelSpaceNormal;
    if (type == u8"b")
        return TextureType::Backlight;
    if (type == u8"sk")
        return TextureType::Skin;
    if (type == u8"em" || type == u8"m")
        return TextureType::EnvironmentMask;

    return std::nullopt;
}

DXGI_FORMAT guess_best_format(const Texture &tex, std::optional<TextureType> type) noexcept
{
    return DXGI_FORMAT_UNKNOWN;
}

} // namespace btu::tex
