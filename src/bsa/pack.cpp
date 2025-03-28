/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/bsa/settings.hpp"

#include <btu/common/algorithms.hpp>
#include <btu/common/functional.hpp>
#include <btu/common/threading.hpp>
#include <flux.hpp>

#include <functional>

namespace btu::bsa {
auto get_allow_file_pred(const PackSettings &sets) -> AllowFilePred
{
    // this part is required, these files would break the archive if packed
    auto is_legal_path = [](const Path &root_dir, const fs::directory_entry &fileinfo) -> bool {
        const bool is_regular = is_regular_file(fileinfo);

        //Removing files at the root directory, those cannot be packed
        const bool is_at_root = equivalent(fileinfo.path().parent_path(), root_dir);

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
    constexpr std::array allowed_types = {FileTypes::Standard, FileTypes::Texture, FileTypes::Incompressible};

    auto packable_files = flux::from_range(fs::recursive_directory_iterator(dir))
                              .filter([&](const auto &p) {
                                  // filter out empty files
                                  return p.is_regular_file() && p.file_size() > 0 && allow_path_pred(dir, p)
                                         && common::contains(allowed_types, get_filetype(p, dir, sets));
                              })
                              .map([](const auto &p) { return p.path(); })
                              .to<std::vector>();

    // sort by size, largest first
    std::ranges::sort(packable_files, [](const auto &lhs, const auto &rhs) {
        return fs::file_size(lhs) > fs::file_size(rhs);
    });

    // if we have separate texture archives, partition textures and standard files
    if (sets.has_texture_version)
    {
        // put textures at the end of the list
        auto [textures_start, _] = std::ranges::stable_partition(packable_files, [&](const auto &file) {
            return get_filetype(file, dir, sets) != FileTypes::Texture;
        });

        return {.standard = std::vector(packable_files.begin(), textures_start),
                .texture  = std::vector(textures_start, packable_files.end())};
    }

    return {.standard = BTU_MOV(packable_files), .texture = {}};
}

[[nodiscard]] auto prepare_file(const Path &file_path,
                                const PackSettings &sets,
                                ArchiveType type) noexcept -> std::optional<File>
{
    auto file = File{sets.game_settings.version, type, get_tes4_archive_type(file_path, sets.game_settings)};
    const bool read_success = file.read(file_path);
    if (!read_success)
        return std::nullopt;

    const bool dx = (file.version() == ArchiveVersion::fo4 || file.version() == ArchiveVersion::starfield)
                    && type == ArchiveType::Textures;

    const bool compressible = get_filetype(file_path, sets.input_dir, sets.game_settings)
                              != FileTypes::Incompressible;

    if ((sets.compress == Compression::Yes && compressible) || dx) // dx is always compressed
    {
        const bool compress_success = file.compress();
        if (!compress_success && dx) // we only care about failure if it's a texture archive
            return std::nullopt;
    }
    return file;
}

[[nodiscard]] auto file_fits(const Archive &arch, const File &file, const Settings &sets) noexcept -> bool
{
    return arch.file_size() + file.size().value_or(0) <= sets.max_size;
}

[[nodiscard]] auto do_pack(std::vector<Path> file_paths,
                           const PackSettings settings,
                           const ArchiveType type) noexcept -> flux::generator<Archive &&>
{
    auto [thread, receiver] = common::make_producer_mt<std::optional<Archive::value_type>>(
        std::move(file_paths), [&](const Path &absolute_path) -> std::optional<Archive::value_type> {
            auto file = prepare_file(absolute_path, settings, type);
            if (!file)
                return std::nullopt;

            auto ret = Archive::value_type{relative(absolute_path, settings.input_dir).string(),
                                           BTU_MOV(file).value()};
            return std::optional{BTU_MOV(ret)};
        });

    auto make_arch = [&] { return Archive{settings.game_settings.version, type}; };
    auto arch      = make_arch();

    for (auto maybe_prepared : receiver)
    {
        if (!maybe_prepared)
            continue; // just ignore this file. TODO: maybe warn?

        auto [relative_path, file] = BTU_MOV(maybe_prepared).value();

        if (file_fits(arch, file, settings.game_settings))
        {
            const bool success = arch.emplace(BTU_MOV(relative_path), BTU_MOV(file));
            assert(success && "file type in bsa mismatch, this should not happen");
            continue;
        }

        // if we are here, the file does not fit into the archive,
        // so we yield the current archive and start a new one
        co_yield std::exchange(arch, make_arch());

        const bool success = arch.emplace(BTU_MOV(relative_path), BTU_MOV(file));
        assert(success && "file type in bsa mismatch, this should not happen");

        // is it even possible that the file is bigger than the max size?
        assert(arch.file_size() <= settings.game_settings.max_size);
    }

    // return the last archive
    if (!arch.empty())
        co_yield BTU_MOV(arch);
}

auto pack(const PackSettings settings) noexcept -> flux::generator<Archive &&>
{
    auto [standard, texture] = list_packable_files(settings.input_dir,
                                                   settings.game_settings,
                                                   get_allow_file_pred(settings));

    if (!standard.empty())
    {
        FLUX_FOR(auto &&a, do_pack(BTU_MOV(standard), settings, ArchiveType::Standard))
        {
            co_yield BTU_MOV(a);
        }
    }

    if (!texture.empty())
    {
        FLUX_FOR(auto &&a, do_pack(BTU_MOV(texture), settings, ArchiveType::Textures))
        {
            co_yield BTU_MOV(a);
        }
    }
}

} // namespace btu::bsa
