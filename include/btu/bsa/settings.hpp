#pragma once

#include "btu/bsa/detail/backends/archive.hpp"
#include "btu/bsa/detail/common.hpp"
#include "btu/common/algorithms.hpp"
#include "btu/common/metaprogramming.hpp"

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
    static const auto inline root = Path("root");

    Path extension;
    std::vector<Path> directories;

    bool check(const Path &filepath, const Path &root) const;
};

struct Settings
{
    Game game;

    std::uintmax_t maxSize;

    ArchiveVersion format;
    std::optional<ArchiveVersion> textureFormat;

    std::optional<Path> suffix;
    std::optional<Path> textureSuffix;

    Path extension;

    std::vector<Path> pluginExtensions;
    std::optional<std::vector<uint8_t>> sDummyPlugin;

    std::vector<AllowedPath> standardFiles;
    std::vector<AllowedPath> textureFiles;
    std::vector<AllowedPath> incompressibleFiles;

    static const Settings &get(Game game);
};

inline const Settings &Settings::get(Game game)
{
    constexpr std::uintmax_t gigabyte = 1024 * 1024 * 1024;

    static Settings defaultSets = [=] {
        Settings sets;
        sets.game             = Game::SSE;
        sets.maxSize          = static_cast<std::uintmax_t>(gigabyte * 1.7); // Safe
        sets.format           = ArchiveVersion::sse;
        sets.textureFormat    = ArchiveVersion::sse;
        sets.textureSuffix    = "Textures";
        sets.extension        = ".bsa";
        sets.pluginExtensions = {".esl", ".esm", ".esp"};
        sets.sDummyPlugin     = std::vector(std::begin(dummy::sse), std::end(dummy::sse));
        sets.standardFiles    = {

            AllowedPath{".bto", {"meshes"}},
            AllowedPath{".btr", {"meshes"}},
            AllowedPath{".btt", {"meshes"}},
            AllowedPath{".dlodsettings", {"lodsettings"}},
            AllowedPath{".dtl", {"meshes"}}, // Unsure
            AllowedPath{".egm", {"meshes"}}, // Unsure
            AllowedPath{".jpg", {"root"}},
            AllowedPath{".hkx", {"meshes"}},
            AllowedPath{".lst", {"meshes"}},
            AllowedPath{".nif", {"meshes"}},
            AllowedPath{".png", {"textures"}},
            AllowedPath{".tga", {"textures"}},
            AllowedPath{".tri", {"meshes"}},
        };
        sets.textureFiles = {
            AllowedPath{".dds", {"textures"}},
        };
        sets.incompressibleFiles = {AllowedPath{".dds", {"interface"}},
                                    AllowedPath{".dlstrings", {"strings"}},
                                    AllowedPath{".fuz", {"sound"}},
                                    AllowedPath{".fxp", {"shadersfx"}},
                                    AllowedPath{".gid", {"grass"}},
                                    AllowedPath{".gfx", {"interface"}},
                                    AllowedPath{".hkc", {"meshes"}},
                                    AllowedPath{".hkt", {"meshes"}},
                                    AllowedPath{".ilstrings", {"strings"}},
                                    AllowedPath{".ini", {"meshes"}},
                                    AllowedPath{".lip", {"sound"}},
                                    AllowedPath{".lnk", {"grass"}},
                                    AllowedPath{".lod", {"lodsettings"}},
                                    AllowedPath{".ogg", {"sound"}},
                                    AllowedPath{".pex", {"scripts"}},
                                    AllowedPath{".psc", {"scripts"}},
                                    AllowedPath{".seq", {"seq"}},
                                    AllowedPath{".strings", {"strings"}},
                                    AllowedPath{".swf", {"interface"}},
                                    AllowedPath{".txt", {"interface", "meshes", "scripts"}},
                                    AllowedPath{".wav", {"sound"}},
                                    AllowedPath{".xml", {"dialogueviews"}},
                                    AllowedPath{".xwm", {"music", "sound", "music"}}};
        return sets;
    }();

    switch (game)
    {
        case Game::TES4:
        {
            static Settings sets = [=] {
                Settings s         = defaultSets;
                s.game             = Game::TES4;
                s.format           = ArchiveVersion::tes4;
                s.textureFormat    = std::nullopt;
                sets.textureSuffix = std::nullopt;
                s.pluginExtensions = {".esm", ".esp"};
                sets.sDummyPlugin  = std::vector(std::begin(dummy::oblivion), std::end(dummy::oblivion));
                return s;
            }();
            return sets;
        }
        case Game::FNV:
        {
            static Settings sets = [=] {
                Settings s         = defaultSets;
                s.game             = Game::FNV;
                s.format           = ArchiveVersion::tes5;
                s.textureFormat    = std::nullopt;
                sets.textureSuffix = std::nullopt;
                s.pluginExtensions = {".esm", ".esp"};
                sets.sDummyPlugin  = std::vector(std::begin(dummy::fnv), std::end(dummy::fnv));
                return s;
            }();
            return sets;
        }
        case Game::SLE:
        {
            static Settings sets = [=] {
                Settings s         = defaultSets;
                s.game             = Game::SLE;
                s.format           = ArchiveVersion::tes5;
                s.textureFormat    = std::nullopt;
                s.suffix           = Path{};
                s.textureSuffix    = std::nullopt;
                s.pluginExtensions = {".esm", ".esp"};
                sets.sDummyPlugin  = std::vector(std::begin(dummy::tes5), std::end(dummy::tes5));
                return s;
            }();
            return sets;
        }
        case Game::SSE: return defaultSets;
        case Game::FO4:
        {
            static Settings sets = [=] {
                Settings s        = defaultSets;
                s.game            = Game::FO4;
                s.format          = ArchiveVersion::fo4;
                s.textureFormat   = ArchiveVersion::fo4dx;
                sets.extension    = ".ba2";
                s.suffix          = "Main";
                sets.sDummyPlugin = std::vector(std::begin(dummy::fo4), std::end(dummy::fo4));
                return s;
            }();
            return sets;
        }
        default: return defaultSets;
    }
}

inline bool AllowedPath::check(const Path &filepath, const Path &root) const
{
    const auto ext = filepath.extension().native();
    if (!common::str_compare(extension.native(), ext, false))
        return false;

    const auto &relative = filepath.lexically_relative(root);
    const auto dir       = [&relative] {
        if (relative.empty())
            return AllowedPath::root;
        else
            return common::to_lower(*relative.begin());
    }();
    return common::contains(directories, dir);

    return true;
}

inline FileTypes get_filetype(const Path &filepath, const Path &root, const Settings &sets)
{
    auto check = [ext = btu::common::to_lower(filepath.extension()), &filepath, &root](const auto &vec) {
        using std::cbegin, std::cend;
        auto it = std::find_if(cbegin(vec),
                               cend(vec),
                               btu::common::overload{[&](const AllowedPath &p) {
                                                         return p.check(filepath, root);
                                                     },
                                                     [&](const auto &p) { return p == ext; }});
        return it != cend(vec);
    };

    if (check(sets.standardFiles))
        return FileTypes::Standard;
    if (check(sets.textureFiles))
        return FileTypes::Texture;
    if (check(sets.incompressibleFiles))
        return FileTypes::Incompressible;
    if (check(sets.pluginExtensions))
        return FileTypes::Plugin;

    Path plugExt[] = {sets.extension};
    if (check(plugExt))
        return FileTypes::BSA;

    return FileTypes::Blacklist;
}
} // namespace btu::bsa
