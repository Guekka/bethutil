/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/bsa/settings.hpp"

#include <btu/common/algorithms.hpp>
#include <btu/common/functional.hpp>
#include <flux.hpp>

#include <deque>
#include <functional>

namespace btu::bsa {
auto get_allow_file_pred(const PackSettings &sets) -> AllowFilePred
{
    // this part is required, these files would break the archive if packed
    auto is_legal_path = [](const Path &root_dir, const fs::directory_entry &fileinfo) -> bool {
        const bool is_regular = fs::is_regular_file(fileinfo);

        //Removing files at the root directory, those cannot be packed
        const bool is_at_root = fs::equivalent(fileinfo.path().parent_path(), root_dir);

        return is_regular && !is_at_root;
    };

    // now we add the user predicate
    return [legal = BTU_MOV(is_legal_path),
            sets  = BTU_MOV(sets)](const Path &root_dir, const fs::directory_entry &fileinfo) -> bool {
        const bool user_allowed = !sets.allow_file_pred || (*sets.allow_file_pred)(root_dir, fileinfo);

        return legal(root_dir, fileinfo) && user_allowed;
    };
}

struct PackGroup
{
    std::vector<Path> standard;
    std::vector<Path> texture;
};

/// \brief List all files in the directory which can be packed, sorted by size (largest first)
[[nodiscard]] auto list_packable_files(const Path &dir,
                                       const Settings &sets,
                                       const AllowFilePred &allow_path_pred) noexcept -> PackGroup
{
    constexpr std::array<FileTypes, 3> allowed_types = {FileTypes::Standard,
                                                        FileTypes::Texture,
                                                        FileTypes::Incompressible};

    auto packable_files = flux::from_range(fs::recursive_directory_iterator(dir))
                              .filter([&](const auto &p) { return allow_path_pred(dir, p); })
                              .filter([&](const auto &p) {
                                  return btu::common::contains(allowed_types,
                                                               get_filetype(p.path(), dir, sets));
                              })
                              .map([](const auto &p) { return p.path(); })
                              .to<std::vector>();

    // sort by size, largest first
    std::ranges::sort(packable_files, [](const auto &lhs, const auto &rhs) {
        return fs::file_size(lhs) > fs::file_size(rhs);
    });

    // if we have separate texture archives, partition textures and standard files
    if (sets.texture_version.has_value())
    {
        // put textures at the end of the list
        auto [textures_start, _] = std::ranges::stable_partition(packable_files, [&](const auto &file) {
            return get_filetype(file, dir, sets) != FileTypes::Texture;
        });

        return {.standard = std::vector<Path>(packable_files.begin(), textures_start),
                .texture  = std::vector<Path>(textures_start, packable_files.end())};
    }

    return {.standard = BTU_MOV(packable_files), .texture = {}};
}

[[nodiscard]] auto get_version(const Settings &sets, ArchiveType type) -> ArchiveVersion
{
    switch (type)
    {
        case ArchiveType::Textures:
        {
            assert(sets.texture_version.has_value());
            return sets.texture_version.value();
        }
        case ArchiveType::Standard:
        {
            return sets.version;
        }
    }
    libbsa::detail::declare_unreachable();
}

[[nodiscard]] auto prepare_file(const Path &file_path, const PackSettings &sets, ArchiveType type) noexcept
    -> File
{
    auto file = File{get_version(sets.game_settings, type)};
    file.read(file_path);

    const bool fo4dx        = file.version() == ArchiveVersion::fo4dx;
    const bool compressible = get_filetype(file_path, sets.input_dir, sets.game_settings)
                              != FileTypes::Incompressible;
    if ((sets.compress == Compression::Yes && compressible) || fo4dx) // fo4dx is always compressed
        file.compress();
    return file;
}

[[nodiscard]] auto file_fits(const Archive &arch, const File &file, const Settings &sets) noexcept -> bool
{
    return arch.file_size() + file.size() <= sets.max_size;
}

[[nodiscard]] auto do_pack(std::vector<Path> file_paths, PackSettings settings, ArchiveType type) noexcept
    -> flux::generator<bsa::Archive &&>
{
    auto [thread, receiver] = common::make_producer_mt<bsa::Archive::value_type>(
        std::move(file_paths), [&](const Path &absolute_path) -> bsa::Archive::value_type {
            auto file = prepare_file(absolute_path, settings, type);
            return {fs::relative(absolute_path, settings.input_dir).string(), BTU_MOV(file)};
        });

    auto make_arch = [&] { return Archive{get_version(settings.game_settings, type), type}; };
    auto arch      = make_arch();

    for (auto [relative_path, file] : receiver)
    {
        if (file_fits(arch, file, settings.game_settings))
        {
            arch.emplace(BTU_MOV(relative_path), BTU_MOV(file));
            continue;
        }

        // if we are here, the file does not fit into the archive,
        // so we yield the current archive and start a new one
        co_yield std::exchange(arch, make_arch());

        arch.emplace(BTU_MOV(relative_path), BTU_MOV(file));

        // is it even possible that the file is bigger than the max size?
        assert(arch.file_size() <= settings.game_settings.max_size);
    }

    // return the last archive
    if (!arch.empty())
        co_yield BTU_MOV(arch);
}

auto pack(PackSettings settings) noexcept -> flux::generator<bsa::Archive &&>
{
    auto files = list_packable_files(settings.input_dir,
                                     settings.game_settings,
                                     get_allow_file_pred(settings));

    if (!files.standard.empty())
    {
        FLUX_FOR(auto &&a, do_pack(BTU_MOV(files.standard), settings, ArchiveType::Standard))
        {
            co_yield BTU_MOV(a);
        }
    }

    if (!files.texture.empty())
    {
        FLUX_FOR(auto &&a, do_pack(BTU_MOV(files.texture), settings, ArchiveType::Textures))
        {
            co_yield BTU_MOV(a);
        }
    }
}

} // namespace btu::bsa
