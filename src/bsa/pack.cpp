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
#include <mutex>

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

[[nodiscard]] auto prepare_file(const Path &file_path, const PackSettings &sets, ArchiveType type) -> File
{
    auto file = [type, &sets] {
        switch (type)
        {
            case ArchiveType::Textures:
            {
                assert(sets.game_settings.texture_version.has_value());
                return File{sets.game_settings.texture_version.value()};
            }
            case ArchiveType::Standard:
            {
                return File{sets.game_settings.version};
            }
        }
        libbsa::detail::declare_unreachable();
    }();
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
    const auto size = archive_size(arch);
    return size + file.size() <= sets.max_size;
}

[[nodiscard]] auto do_write(Archive arch, const PackSettings &settings, ArchiveType type)
{
    auto filepath = settings.output_dir / settings.archive_name_gen(type);

    write_archive(BTU_MOV(arch), BTU_MOV(filepath));
}

[[nodiscard]] auto do_pack(std::vector<Path> files, const PackSettings &settings, ArchiveType type) noexcept
    -> std::vector<std::pair<Path, std::exception_ptr>>
{
    auto arch = Archive{};
    auto ret  = std::vector<std::pair<Path, std::exception_ptr>>{};

    std::mutex mut;
    btu::common::for_each_mt(BTU_MOV(files), [&settings, &mut, &arch, type, &ret](Path &&absolute_path) {
        try
        {
            auto file = prepare_file(absolute_path, settings, type);

            std::unique_lock lock(mut);
            if (file_fits(arch, file, settings.game_settings))
            {
                arch.emplace(fs::relative(absolute_path, settings.input_dir).string(), BTU_MOV(file));
                return;
            }

            // if we are here, the file does not fit into the archive
            // we need to write the archive and start a new one
            // to avoid locking the mutex for too long, we prepare the new archive first
            auto full_arch = BTU_MOV(arch);

            arch = Archive{};
            arch.emplace(fs::relative(absolute_path, settings.input_dir).string(), BTU_MOV(file));

            assert(archive_size(arch) <= settings.game_settings.max_size);

            // it is now safe to unlock the mutex, the full archive is not shared with other threads
            lock.unlock();

            do_write(BTU_MOV(full_arch), settings, type);
        }
        catch (const std::exception &e)
        {
            const std::unique_lock lock(mut);
            ret.emplace_back(BTU_MOV(absolute_path), std::make_exception_ptr(e));
        }
    });

    // write the last archive
    try
    {
        if (!arch.empty())
            do_write(BTU_MOV(arch), settings, type);
    }
    catch (const std::exception &e)
    {
        ret.emplace_back(settings.archive_name_gen(type), std::make_exception_ptr(e));
    }

    return ret;
}

auto pack(const PackSettings &settings) noexcept -> std::vector<std::pair<Path, std::exception_ptr>>
{
    auto files = list_packable_files(settings.input_dir,
                                     settings.game_settings,
                                     get_allow_file_pred(settings));

    auto ret = std::vector<std::pair<Path, std::exception_ptr>>{};

    if (!files.standard.empty())
        ret = do_pack(BTU_MOV(files.standard), settings, ArchiveType::Standard);

    if (!files.texture.empty())
    {
        auto errs = do_pack(BTU_MOV(files.texture), settings, ArchiveType::Textures);
        ret.insert(ret.end(), std::make_move_iterator(errs.begin()), std::make_move_iterator(errs.end()));
    }

    return ret;
}

} // namespace btu::bsa
