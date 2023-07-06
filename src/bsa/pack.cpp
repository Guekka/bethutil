/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/bsa/settings.hpp"

#include <btu/common/algorithms.hpp>
#include <btu/common/functional.hpp>
#include <flow.hpp>

#include <deque>
#include <execution>
#include <fstream>
#include <functional>

namespace btu::bsa {
auto default_is_allowed_path(const Path &dir, fs::directory_entry const &fileinfo) -> bool
{
    bool const is_regular = fs::is_regular_file(fileinfo);

    //Removing files at the root directory, those cannot be packed
    bool const is_at_root = fileinfo.path().parent_path() == dir;

    return is_regular && !is_at_root;
}

auto write(Path filepath, Compression compressed, ArchiveData &&data)
    -> std::vector<std::pair<Path, std::string>>
{
    if (data.empty())
        return {}; // Do not write empty archive

    if (data.get_version() == ArchiveVersion::fo4dx)
        compressed = Compression::Yes; // fo4dx has to be compressed

    auto arch      = Archive{};
    auto ret       = std::vector<std::pair<Path, std::string>>();
    auto root_path = data.get_root_path();
    const auto ver = data.get_version();
    std::mutex mut;
    btu::common::for_each_mt(std::move(data), [&](Path &&relative_path) {
        try
        {
            auto file = File(ver);
            file.read(root_path / relative_path);
            if (compressed == Compression::Yes)
                file.compress();

            const std::lock_guard lock(mut);
            arch.emplace(std::move(relative_path).string(), std::move(file));
        }
        catch (const std::exception &e)
        {
            ret.emplace_back(relative_path, e.what());
        }
    });
    write_archive(std::move(arch), std::move(filepath));
    return ret;
}

struct PackableFile
{
    Path absolute_path;
    uint64_t size;
};

[[nodiscard]] auto list_packable_files(const Path &dir,
                                       const Settings &sets,
                                       const AllowFilePred &allow_path_pred) -> std::vector<PackableFile>
{
    constexpr std::array<FileTypes, 3> allowed_types = {FileTypes::Standard,
                                                        FileTypes::Texture,
                                                        FileTypes::Incompressible};

    return flow::from(fs::recursive_directory_iterator(dir))
        .filter([&](const auto &p) { return allow_path_pred(dir, p); })
        .filter([&](const auto &p) {
            return btu::common::contains(allowed_types, get_filetype(p.path(), dir, sets));
        })
        .map([&](const auto &p) {
            return PackableFile{p.path(), fs::file_size(p.path())};
        })
        .to_vector();
}

template<std::ranges::range R>
requires std::is_same_v<std::ranges::range_value_t<R>, PackableFile>
[[nodiscard]] auto do_pack(const R &packable_files, const Path &dir, const Settings &sets, ArchiveType type)
    -> std::vector<ArchiveData>
{
    auto ret = std::vector<ArchiveData>{};
    for (auto &&file : packable_files)
    {
        bool added = false;
        // try to add to existing archive
        for (auto &archive : ret)
        {
            if (archive.add_file(file.absolute_path, file.size))
            {
                added = true;
                break;
            }
        }
        if (added)
            continue;

        // if we couldn't add to any existing archive, create a new one
        ret.emplace_back(sets, type, dir);
        ret.back().add_file(file.absolute_path, file.size);
    }
    return ret;
}

auto prepare_archive(const Path &dir, const Settings &sets, const AllowFilePred &allow_path_pred)
    -> std::vector<ArchiveData>
{
    auto packable_files = list_packable_files(dir, sets, allow_path_pred);
    // sort by size, largest first
    std::ranges::sort(packable_files, {}, &PackableFile::size);
    std::ranges::reverse(packable_files);
    // now we have the largest files first, we can start packing them. There are two cases:
    // games with separate texture archives and games with one archive for everything

    auto ret = std::vector<ArchiveData>{};

    // if we have separate texture archives, we can pack textures separately
    if (sets.texture_format.has_value())
    {
        // put textures at the end of the list
        auto textures = std::ranges::stable_partition(packable_files, [&](const auto &file) {
            return get_filetype(file.absolute_path, dir, sets) != FileTypes::Texture;
        });

        ret = do_pack(textures, dir, sets, ArchiveType::Textures);

        // remove textures from packable_files
        packable_files.erase(textures.begin(), textures.end());
    }
    auto standard = do_pack(packable_files, dir, sets, ArchiveType::Standard);
    ret.insert(ret.end(), standard.begin(), standard.end());

    return ret;
}

} // namespace btu::bsa
