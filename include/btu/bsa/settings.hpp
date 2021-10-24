#pragma once

#include "btu/bsa/detail/backends/archive.hpp"
#include "btu/bsa/detail/common.hpp"
#include "btu/common/algorithms.hpp"
#include "btu/common/metaprogramming.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <variant>

namespace btu::bsa {
namespace dummy {
static constexpr uint8_t tes5[] = {0x54, 0x45, 0x53, 0x34, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x2B, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00,
                                   0x9A, 0x99, 0xD9, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                   0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x01, 0x00, 0x00};

static constexpr uint8_t sse[] = {0x54, 0x45, 0x53, 0x34, 0x19, 0x00, 0x00, 0x00, 0x00, 0x02,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x2C, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00,
                                  0x9A, 0x99, 0xD9, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                  0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x01, 0x00, 0x00};

static constexpr uint8_t oblivion[] = {0x54, 0x45, 0x53, 0x34, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x45,
                                       0x44, 0x52, 0x0C, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x08, 0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x08, 0x00,
                                       0x44, 0x45, 0x46, 0x41, 0x55, 0x4C, 0x54, 0x00};

static constexpr uint8_t fnv[] = {0x54, 0x45, 0x53, 0x34, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x0F, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00,
                                  0x1F, 0x85, 0xAB, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                  0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x01, 0x00, 0x00};

static constexpr uint8_t fo4[] = {0x54, 0x45, 0x53, 0x34, 0x19, 0x00, 0x00, 0x00, 0x00, 0x02,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x83, 0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x0C, 0x00,
                                  0x33, 0x33, 0x73, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                                  0x00, 0x00, 0x43, 0x4E, 0x41, 0x4D, 0x01, 0x00, 0x00};
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

struct AllowedPath
{
    static const inline auto k_root = std::u8string(u8"root");

    std::u8string extension;
    std::vector<std::u8string> directories;

    [[nodiscard]] auto check(const Path &filepath, const Path &root) const -> bool;
};

struct Settings
{
    Game game{};

    std::uintmax_t max_size{};

    ArchiveVersion format{};
    std::optional<ArchiveVersion> texture_format;

    std::optional<std::u8string> suffix;
    std::optional<std::u8string> texture_suffix;

    std::u8string extension;

    std::vector<std::u8string> plugin_extensions;
    std::optional<std::vector<uint8_t>> s_dummy_plugin;

    std::vector<AllowedPath> standard_files;
    std::vector<AllowedPath> texture_files;
    std::vector<AllowedPath> incompressible_files;

    [[nodiscard]] static auto get(Game game) -> const Settings &;
};

[[nodiscard]] inline auto Settings::get(Game game) -> const Settings &
{
    constexpr auto megabyte = 1024UL * 1024UL;

    static const Settings default_sets = [=] {
        Settings sets;
        sets.game              = Game::SSE;
        sets.max_size          = 2000 * megabyte;
        sets.format            = ArchiveVersion::sse;
        sets.texture_format    = ArchiveVersion::sse;
        sets.texture_suffix    = u8"Textures";
        sets.extension         = u8".bsa";
        sets.plugin_extensions = {u8".esl", u8".esm", u8".esp"};
        sets.s_dummy_plugin    = std::vector(std::begin(dummy::sse), std::end(dummy::sse));
        sets.standard_files    = {
            AllowedPath{u8".bgem", {u8"materials"}},
            AllowedPath{u8".bgsm", {u8"materials"}},
            AllowedPath{u8".bto", {u8"meshes"}},
            AllowedPath{u8".btr", {u8"meshes"}},
            AllowedPath{u8".btt", {u8"meshes"}},
            AllowedPath{u8".dlodsettings", {u8"lodsettings"}},
            AllowedPath{u8".dtl", {u8"meshes"}}, // Unsure
            AllowedPath{u8".egm", {u8"meshes"}}, // Unsure
            AllowedPath{u8".jpg", {u8"root"}},
            AllowedPath{u8".hkx", {u8"meshes"}},
            AllowedPath{u8".lst", {u8"meshes"}},
            AllowedPath{u8".nif", {u8"meshes"}},
            AllowedPath{u8".psc", {u8"scripts", u8"source"}},
            AllowedPath{u8".tga", {u8"textures"}},
            AllowedPath{u8".tri", {u8"meshes"}},
        };
        sets.texture_files = {
            AllowedPath{u8".dds", {u8"textures", u8"interface"}},
            AllowedPath{u8".png", {u8"textures"}},
        };
        sets.incompressible_files = {AllowedPath{u8".dlstrings", {u8"strings"}},
                                     AllowedPath{u8".fuz", {u8"sound"}},
                                     AllowedPath{u8".fxp", {u8"shadersfx"}},
                                     AllowedPath{u8".gid", {u8"grass"}},
                                     AllowedPath{u8".gfx", {u8"interface"}},
                                     AllowedPath{u8".hkc", {u8"meshes"}},
                                     AllowedPath{u8".hkt", {u8"meshes"}},
                                     AllowedPath{u8".ilstrings", {u8"strings"}},
                                     AllowedPath{u8".ini", {u8"meshes"}},
                                     AllowedPath{u8".lip", {u8"sound"}},
                                     AllowedPath{u8".lnk", {u8"grass"}},
                                     AllowedPath{u8".lod", {u8"lodsettings"}},
                                     AllowedPath{u8".ogg", {u8"sound"}},
                                     AllowedPath{u8".pex", {u8"scripts"}},
                                     AllowedPath{u8".seq", {u8"seq"}},
                                     AllowedPath{u8".strings", {u8"strings"}},
                                     AllowedPath{u8".swf", {u8"interface"}},
                                     AllowedPath{u8".txt", {u8"interface", u8"meshes", u8"scripts"}},
                                     AllowedPath{u8".wav", {u8"sound"}},
                                     AllowedPath{u8".xml", {u8"dialogueviews"}},
                                     AllowedPath{u8".xwm", {u8"music", u8"sound"}}};
        return sets;
    }();

    switch (game)
    {
        case Game::TES4:
        {
            static const Settings sets_tes4 = [=] {
                Settings s          = default_sets;
                s.game              = Game::TES4;
                s.format            = ArchiveVersion::tes4;
                s.texture_format    = std::nullopt;
                s.texture_suffix    = std::nullopt;
                s.plugin_extensions = {u8".esm", u8".esp"};
                s.s_dummy_plugin    = std::vector(std::begin(dummy::oblivion), std::end(dummy::oblivion));
                return s;
            }();
            return sets_tes4;
        }
        case Game::FNV:
        {
            static const Settings sets_fnv = [=] {
                Settings s          = default_sets;
                s.game              = Game::FNV;
                s.format            = ArchiveVersion::tes5;
                s.texture_format    = std::nullopt;
                s.texture_suffix    = std::nullopt;
                s.plugin_extensions = {u8".esm", u8".esp"};
                s.s_dummy_plugin    = std::vector(std::begin(dummy::fnv), std::end(dummy::fnv));
                return s;
            }();
            return sets_fnv;
        }
        case Game::SLE:
        {
            static const Settings sets_sle = [=] {
                Settings s          = default_sets;
                s.game              = Game::SLE;
                s.format            = ArchiveVersion::tes5;
                s.texture_format    = std::nullopt;
                s.suffix            = {};
                s.texture_suffix    = std::nullopt;
                s.plugin_extensions = {u8".esm", u8".esp"};
                s.s_dummy_plugin    = std::vector(std::begin(dummy::tes5), std::end(dummy::tes5));
                return s;
            }();
            return sets_sle;
        }
        case Game::SSE: return default_sets;
        case Game::FO4:
        {
            static const Settings sets_fo4 = [=] {
                Settings s       = default_sets;
                s.game           = Game::FO4;
                s.format         = ArchiveVersion::fo4;
                s.texture_format = ArchiveVersion::fo4dx;
                s.max_size       = 4000 * megabyte;
                s.extension      = u8".ba2";
                s.suffix         = u8"Main";
                s.texture_files  = {AllowedPath{u8".dds", {u8"textures", u8"interface"}}};
                s.standard_files.emplace_back(AllowedPath{u8".png", {u8"textures"}});
                s.s_dummy_plugin = std::vector(std::begin(dummy::fo4), std::end(dummy::fo4));
                return s;
            }();
            return sets_fo4;
        }
        default: return default_sets;
    }
}

[[nodiscard]] inline auto AllowedPath::check(const Path &filepath, const Path &root) const -> bool
{
    const auto ext = filepath.extension().u8string();
    if (!common::str_compare(extension, ext, /*case_sensitive=*/false))
        return false;

    const auto &relative = filepath.lexically_relative(root);
    const auto dir       = [&relative] {
        if (relative.empty())
            return AllowedPath::k_root;

        return common::to_lower(relative.begin()->u8string());
    }();

    return common::contains(directories, dir);
}

[[nodiscard]] inline auto get_filetype(const Path &filepath, const Path &root, const Settings &sets)
    -> FileTypes
{
    const auto ext = btu::common::to_lower(filepath.extension().u8string());
    auto check     = [ext, &filepath, &root](const auto &vec) {
        using std::cbegin, std::cend;
        auto it = std::find_if(cbegin(vec),
                               cend(vec),
                               btu::common::overload{[&](const AllowedPath &p) {
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
} // namespace btu::bsa
