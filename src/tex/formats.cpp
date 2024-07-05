#include <btu/common/string.hpp>
#include <btu/tex/formats.hpp>

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
    const auto type = common::to_lower(path);

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

auto guess_best_format(const DXGI_FORMAT current_format,
                       const BestFormatFor formats,
                       const AllowCompressed allow_compressed,
                       const ForceAlpha force_alpha) noexcept -> DXGI_FORMAT
{
    // allow compression if user requested it, or it was already compressed
    const bool compressed = allow_compressed == AllowCompressed::Yes || DirectX::IsCompressed(current_format);
    // provide an alpha channel if there was already one
    const bool alpha = DirectX::HasAlpha(current_format) || force_alpha == ForceAlpha::Yes;
    if (compressed && alpha)
        return formats.compressed;
    if (compressed)
        return formats.compressed_without_alpha;
    if (!alpha)
        return formats.uncompressed_without_alpha;

    return formats.uncompressed;
}

} // namespace btu::tex
