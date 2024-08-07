#pragma once

#include "btu/bsa/archive.hpp"
#include "btu/common/algorithms.hpp"
#include "btu/common/games.hpp"
#include "btu/common/metaprogramming.hpp"

#include <bsa/bsa.hpp>
#include <btu/common/json.hpp>

#include <array>
#include <iostream>
#include <optional>
#include <variant>

namespace btu::bsa {
namespace dummy {

template<size_t N>
[[nodiscard]] constexpr auto as_bytes(std::array<uint8_t, N> data) -> std::array<std::byte, N>
{
    std::array<std::byte, N> result{};
    for (size_t i = 0; i < N; ++i)
        result[i] = std::byte{data[i]};
    return result;
}

static constexpr auto tes5 = as_bytes(std::to_array<uint8_t>(
    {0x54, 0x45, 0x53, 0x34, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00, 0x9A, 0x99, 0xD9, 0x3F,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x01, 0x00, 0x00}));

static constexpr auto sse = as_bytes(std::to_array<uint8_t>(
    {0x54, 0x45, 0x53, 0x34, 0x19, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00, 0x9A, 0x99, 0xD9, 0x3F,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x01, 0x00, 0x00}));

static constexpr auto oblivion = as_bytes(
    std::to_array<uint8_t>({0x54, 0x45, 0x53, 0x34, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00,
                            0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x43,
                            0x4E, 0x41, 0x4D, 0x08, 0x00, 0x44, 0x45, 0x46, 0x41, 0x55, 0x4C, 0x54, 0x00}));

static constexpr auto fnv = as_bytes(std::to_array<uint8_t>(
    {0x54, 0x45, 0x53, 0x34, 0x3b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00, 0x1F, 0x85, 0xAB, 0x3F,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x01, 0x00, 0x00, 0x4D, 0x41,
     0x53, 0x54, 0x0E, 0x00, 0x46, 0x61, 0x6C, 0x6C, 0x6F, 0x75, 0x74, 0x4E, 0x56, 0x2E, 0x65, 0x73, 0x6D,
     0x00, 0x44, 0x41, 0x54, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));

static constexpr auto fo4 = as_bytes(std::to_array<uint8_t>(
    {0x54, 0x45, 0x53, 0x34, 0x19, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00, 0x33, 0x33, 0x73, 0x3F,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x01, 0x00, 0x00}));

static constexpr auto starfield = as_bytes(std::to_array<uint8_t>(
    {0x54, 0x45, 0x53, 0x34, 0x50, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x02, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00,
     0x8F, 0xC2, 0x75, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x43, 0x4E, 0x41,
     0x4D, 0x01, 0x00, 0x00, 0x4D, 0x41, 0x53, 0x54, 0x0E, 0x00, 0x53, 0x74, 0x61, 0x72, 0x66,
     0x69, 0x65, 0x6C, 0x64, 0x2E, 0x65, 0x73, 0x6D, 0x00, 0x4D, 0x41, 0x53, 0x54, 0x1D, 0x00,
     0x42, 0x6C, 0x75, 0x65, 0x70, 0x72, 0x69, 0x6E, 0x74, 0x53, 0x68, 0x69, 0x70, 0x73, 0x2D,
     0x53, 0x74, 0x61, 0x72, 0x66, 0x69, 0x65, 0x6C, 0x64, 0x2E, 0x65, 0x73, 0x6D, 0x00}));
} // namespace dummy

enum class FileTypes
{
    Standard,
    Texture,
    Incompressible,
    Blacklist,
    Plugin,
    BSA
};

NLOHMANN_JSON_SERIALIZE_ENUM(FileTypes,
                             {{FileTypes::Standard, "standard"},
                              {FileTypes::Texture, "texture"},
                              {FileTypes::Incompressible, "incompressible"},
                              {FileTypes::Blacklist, "blacklist"},
                              {FileTypes::Plugin, "plugin"},
                              {FileTypes::BSA, "bsa"}})

static constexpr auto k_bsa_ext = u8".bsa";
static constexpr auto k_ba2_ext = u8".ba2";

static constexpr auto k_archive_extensions = std::to_array<std::u8string_view>({k_bsa_ext, k_ba2_ext});

enum class PluginLoadingMode
{
    Limited,  // Plugins only load archives "<plugin>[ - Suffix]" and "<plugin> - Textures".
    Unlimited // Plugins load archives "<plugin>*".
};

NLOHMANN_JSON_SERIALIZE_ENUM(PluginLoadingMode,
                             {{PluginLoadingMode::Limited, "limited"},
                              {PluginLoadingMode::Unlimited, "unlimited"}});

struct AllowedPath
{
    static const inline auto k_root = std::u8string(u8"root");

    std::u8string extension;
    std::vector<std::u8string> directories;
    std::optional<TES4ArchiveType> tes4_archive_type;

    [[nodiscard]] auto check(const Path &filepath, const Path &root) const -> bool;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AllowedPath, extension, directories)

struct Settings
{
    Game game{};

    uint64_t max_size{};

    ArchiveVersion version{};
    bool has_texture_version;

    std::optional<std::u8string> suffix;
    std::optional<std::u8string> texture_suffix;

    std::u8string extension;

    std::vector<std::u8string> plugin_extensions;
    std::optional<std::vector<std::byte>> dummy_plugin;
    std::u8string dummy_extension;
    PluginLoadingMode dummy_plugin_loading_mode{};

    std::vector<AllowedPath> standard_files;
    std::vector<AllowedPath> texture_files;
    std::vector<AllowedPath> incompressible_files;

    [[nodiscard]] static auto get(Game game) -> const Settings &;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Settings,
                                   game,
                                   max_size,
                                   version,
                                   has_texture_version,
                                   suffix,
                                   texture_suffix,
                                   extension,
                                   plugin_extensions,
                                   dummy_plugin,
                                   standard_files,
                                   texture_files,
                                   incompressible_files)

[[nodiscard]] inline auto Settings::get(Game game) -> const Settings &
{
    constexpr auto megabyte = 1024ULL * 1024ULL;

    static const Settings tes4_default_sets = [=] {
        Settings sets;
        sets.game                      = Game::TES4;
        sets.max_size                  = 2000ULL * megabyte;
        sets.version                   = ArchiveVersion::tes4;
        sets.has_texture_version       = false;
        sets.texture_suffix            = std::nullopt;
        sets.extension                 = u8".bsa";
        sets.plugin_extensions         = {u8".esm", u8".esp"};
        sets.dummy_plugin              = std::vector(std::begin(dummy::oblivion), std::end(dummy::oblivion));
        sets.dummy_extension           = u8".esp";
        sets.dummy_plugin_loading_mode = PluginLoadingMode::Unlimited;
        sets.standard_files            = {
            {u8".nif", {u8"meshes"}, TES4ArchiveType::meshes},
            {u8".egm", {u8"meshes"}, TES4ArchiveType::meshes},
            {u8".egt", {u8"meshes"}, TES4ArchiveType::meshes},
            {u8".tri", {u8"meshes"}, TES4ArchiveType::meshes},
            {u8".cmp", {u8"meshes"}, TES4ArchiveType::meshes},
            {u8".lst", {u8"meshes"}, TES4ArchiveType::meshes},
            {u8".dtl", {u8"meshes"}, TES4ArchiveType::meshes},
            {u8".spt", {u8"trees"}, TES4ArchiveType::meshes},
        };
        sets.texture_files = {
            {u8".dds", {u8"textures"}, TES4ArchiveType::textures},
            {u8".tai", {u8"textures"}, TES4ArchiveType::textures},
            {u8".tga", {u8"textures"}, TES4ArchiveType::textures},
            {u8".bmp", {u8"textures"}, TES4ArchiveType::textures},
            {u8".fnt", {u8"textures"}, TES4ArchiveType::fonts},
            {u8".tex", {u8"textures"}, TES4ArchiveType::fonts},
        };
        sets.incompressible_files = {
            {u8".wav", {u8"sound"}, TES4ArchiveType::sounds},
            {u8".ogg", {u8"sound"}, TES4ArchiveType::voices},
            {u8".lip", {u8"sound"}, TES4ArchiveType::voices},
            {u8".txt", {u8"menus"}, TES4ArchiveType::menus},
            {u8".vso", {u8"shaders"}, TES4ArchiveType::shaders},
            {u8".pso", {u8"shaders"}, TES4ArchiveType::shaders},
            {u8".vsh", {u8"shaders"}, TES4ArchiveType::shaders},
            {u8".psh", {u8"shaders"}, TES4ArchiveType::shaders},
            {u8".lsl", {u8"shaders"}, TES4ArchiveType::shaders},
            {u8".h", {u8"shaders"}, TES4ArchiveType::shaders},
            {u8".dat", {u8"lsdata"}, TES4ArchiveType::misc},
            {u8".dlodsettings", {u8"lodsettings"}, TES4ArchiveType::misc},
            {u8".ctl", {u8"facegen"}, TES4ArchiveType::misc},
        };
        // Purposefully missing: .kf, .mp3, .ini, .txt, .json
        return sets;
    }();

    static const Settings tes5_default_sets = [=] {
        Settings sets                  = tes4_default_sets;
        sets.game                      = Game::SSE;
        sets.version                   = ArchiveVersion::sse;
        sets.has_texture_version       = true;
        sets.texture_suffix            = u8"Textures";
        sets.extension                 = u8".bsa";
        sets.plugin_extensions         = {u8".esl", u8".esm", u8".esp"};
        sets.dummy_plugin              = std::vector(std::begin(dummy::sse), std::end(dummy::sse));
        sets.dummy_plugin_loading_mode = PluginLoadingMode::Limited;
        sets.standard_files            = {
            AllowedPath{u8".bgem", {u8"materials"}},
            AllowedPath{u8".bgsm", {u8"materials"}},
            AllowedPath{u8".bto", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".btr", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".btt", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".cgid", {u8"grass"}},
            AllowedPath{u8".dlodsettings", {u8"lodsettings"}, TES4ArchiveType::misc},
            AllowedPath{u8".dtl", {u8"meshes"}, TES4ArchiveType::meshes}, // Unsure
            AllowedPath{u8".egm", {u8"meshes"}, TES4ArchiveType::meshes}, // Unsure
            AllowedPath{u8".jpg", {u8"root"}},
            AllowedPath{u8".hkb", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".hkb", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".hkx", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".lst", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".nif", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".psc", {u8"scripts", u8"source"}, TES4ArchiveType::misc},
            AllowedPath{u8".tga", {u8"textures"}, TES4ArchiveType::textures},
            AllowedPath{u8".tri", {u8"meshes"}, TES4ArchiveType::meshes},
        };
        sets.texture_files = {
            AllowedPath{u8".dds", {u8"textures", u8"interface"}, TES4ArchiveType::textures},
            AllowedPath{u8".png", {u8"textures"}, TES4ArchiveType::textures},
        };
        sets.incompressible_files = {
            AllowedPath{u8".dlstrings", {u8"strings"}},
            AllowedPath{u8".fuz", {u8"sound"}, TES4ArchiveType::sounds},
            AllowedPath{u8".fxp", {u8"shadersfx"}, TES4ArchiveType::shaders},
            AllowedPath{u8".gid", {u8"grass"}},
            AllowedPath{u8".gfx", {u8"interface"}, TES4ArchiveType::menus},
            AllowedPath{u8".hkc", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".hkt", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".hkp", {u8"meshes"}, TES4ArchiveType::meshes},
            AllowedPath{u8".ilstrings", {u8"strings"}},
            AllowedPath{u8".ini", {u8"meshes"}},
            AllowedPath{u8".lip", {u8"sound"}, TES4ArchiveType::voices},
            AllowedPath{u8".lnk", {u8"grass"}},
            AllowedPath{u8".lod", {u8"lodsettings"}},
            AllowedPath{u8".ogg", {u8"sound"}, TES4ArchiveType::voices},
            AllowedPath{u8".pex", {u8"scripts"}, TES4ArchiveType::misc},
            AllowedPath{u8".seq", {u8"seq"}},
            AllowedPath{u8".strings", {u8"strings"}},
            AllowedPath{u8".swf", {u8"interface"}},
            AllowedPath{u8".txt", {u8"interface", u8"meshes", u8"scripts"}},
            AllowedPath{u8".wav", {u8"sound"}, TES4ArchiveType::sounds},
            AllowedPath{u8".xml", {u8"dialogueviews"}, TES4ArchiveType::menus},
            AllowedPath{u8".xwm", {u8"music", u8"sound"}, TES4ArchiveType::sounds},
        };
        return sets;
    }();

    switch (game)
    {
        case Game::TES4: return tes4_default_sets;
        case Game::FNV:
        {
            static const Settings sets_fnv = [=] {
                Settings s     = tes4_default_sets;
                s.game         = Game::FNV;
                s.version      = ArchiveVersion::tes5;
                s.dummy_plugin = std::vector(std::begin(dummy::fnv), std::end(dummy::fnv));
                return s;
            }();
            return sets_fnv;
        }
        case Game::SLE:
        {
            static const Settings sets_sle = [=] {
                Settings s            = tes5_default_sets;
                s.game                = Game::SLE;
                s.version             = ArchiveVersion::tes5;
                s.has_texture_version = false;
                s.suffix              = {};
                s.texture_suffix      = std::nullopt;
                s.plugin_extensions   = {u8".esm", u8".esp"};
                s.dummy_plugin        = std::vector(std::begin(dummy::tes5), std::end(dummy::tes5));
                return s;
            }();
            return sets_sle;
        }
        case Game::SSE: return tes5_default_sets;
        case Game::FO4:
        {
            static const Settings sets_fo4 = [=] {
                Settings s            = tes5_default_sets;
                s.game                = Game::FO4;
                s.version             = ArchiveVersion::fo4;
                s.max_size            = 4000ULL * megabyte;
                s.extension           = u8".ba2";
                s.suffix              = u8"Main";
                s.texture_files       = {AllowedPath{u8".dds", {u8"textures", u8"interface"}}};
                s.has_texture_version = true;
                s.standard_files.emplace_back(AllowedPath{u8".png", {u8"textures"}});
                s.standard_files.emplace_back(AllowedPath{u8".uvd", {u8"vis"}});
                s.dummy_plugin = std::vector(std::begin(dummy::fo4), std::end(dummy::fo4));
                return s;
            }();
            return sets_fo4;
        }
        case Game::Starfield:
        {
            static const Settings sets_starfield = [=] {
                Settings s            = get(Game::FO4);
                s.game                = Game::Starfield;
                s.version             = ArchiveVersion::starfield;
                s.has_texture_version = true;
                s.dummy_plugin        = std::vector(std::begin(dummy::starfield), std::end(dummy::starfield));
                s.dummy_extension     = u8".esm";
                return s;
            }();
            return sets_starfield;
        }
        default: return tes5_default_sets;
    }
}

[[nodiscard]] inline auto AllowedPath::check(const Path &filepath, const Path &root) const -> bool
{
    const auto ext = filepath.extension().u8string();
    if (!str_compare(extension, ext, common::CaseSensitive::No))
        return false;

    const auto &relative = filepath.lexically_relative(root);
    const auto dir       = [&relative] {
        if (relative.empty())
            return k_root;

        return common::to_lower(relative.begin()->u8string());
    }();

    return common::contains(directories, dir);
}

[[nodiscard]] inline auto get_filetype(const Path &filepath,
                                       const Path &root,
                                       const Settings &sets) -> FileTypes
{
    const auto ext = common::to_lower(filepath.extension().u8string());
    auto check     = [ext, &filepath, &root](const auto &vec) {
        using std::cbegin, std::cend;
        auto it = std::ranges::find_if(vec,
                                       common::Overload{[&](const AllowedPath &p) {
                                                            return p.check(filepath, root);
                                                        },
                                                        [&](const auto &p) { return p == ext; }});
        return it != cend(vec);
    };

    if (check(sets.standard_files))
        return FileTypes::Standard;
    if (check(sets.texture_files))
        return FileTypes::Texture;
    if (check(sets.incompressible_files))
        return FileTypes::Incompressible;
    if (check(sets.plugin_extensions))
        return FileTypes::Plugin;

    auto plug_ext = std::to_array({sets.extension});
    if (check(plug_ext))
        return FileTypes::BSA;

    return FileTypes::Blacklist;
}

[[nodiscard]] inline auto get_tes4_archive_type(const Path &filepath,
                                                const Settings &sets) -> std::optional<TES4ArchiveType>
{
    const auto ext = common::to_lower(filepath.extension().u8string());
    auto get       = [ext](const auto &vec) {
        using std::cbegin, std::cend;
        auto it = std::ranges::find_if(vec, [&](const auto &p) { return p.extension == ext; });
        if (it != cend(vec))
        {
            return (*it).tes4_archive_type;
        }
        return std::optional<TES4ArchiveType>(std::nullopt);
    };

    if (auto arch_type = get(sets.standard_files); arch_type)
        return arch_type;
    if (auto arch_type = get(sets.texture_files); arch_type)
        return arch_type;
    if (auto arch_type = get(sets.incompressible_files); arch_type)
        return arch_type;

    return std::nullopt;
}
} // namespace btu::bsa
