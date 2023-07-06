/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/bsa/settings.hpp"

#include <btu/common/algorithms.hpp>
#include <btu/common/functional.hpp>

#include <deque>
#include <execution>
#include <fstream>
#include <functional>

namespace btu::bsa {
auto default_is_allowed_path(const Path &dir, fs::directory_entry const &fileinfo) -> bool
{
    bool const is_dir = fileinfo.is_directory();

    //Removing files at the root directory, those cannot be packed
    bool const is_root = fileinfo.path().parent_path() == dir;

    return !is_dir && !is_root;
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

auto split(const Path &dir, const Settings &sets, const AllowFilePred &allow_path_pred)
    -> std::vector<ArchiveData>
{
    auto standard = ArchiveData(sets, ArchiveType::Standard, dir);
    auto textures = ArchiveData(sets, ArchiveType::Textures, dir);

    std::vector<ArchiveData> res;

    for (const auto &p : fs::recursive_directory_iterator(dir))
    {
        if (!allow_path_pred(dir, p))
            continue;

        const auto ft = get_filetype(p.path(), dir, sets);
        if (ft != FileTypes::Standard && ft != FileTypes::Texture && ft != FileTypes::Incompressible)
            continue;

        auto *p_bsa = &standard; // everything except textures goes to standard
        if (ft == FileTypes::Texture)
            p_bsa = &textures;

        //adding files and sizes to list
        if (!p_bsa->add_file(p.path()))
        {
            // BSA full, write it
            res.emplace_back(std::move(*p_bsa));

            // Get a new one and add the file that did not make it
            *p_bsa = ArchiveData(sets, p_bsa->get_type(), dir);
            p_bsa->add_file(p.path());
        }
    }

    // Add BSAs that are not full
    if (!standard.empty())
        res.emplace_back(std::move(standard));
    if (!textures.empty())
        res.emplace_back(std::move(textures));
    return res;
}

void merge(std::vector<ArchiveData> &archives, MergeFlags flags)
{
    const auto test_flag = [&](MergeFlags flag) { return btu::common::to_underlying(flags & flag) != 0; };

    // We have at most one underfull BSA per type, so we only consider the last two BSAs
    if (archives.size() < 2)
        return; // Nothing to merge

    auto standard = archives.end() - 2;
    auto textures = archives.end() - 1;

    if (standard->get_type() != ArchiveType::Standard || textures->get_type() != ArchiveType::Textures)
        return; // Nothing to merge: at least one of them is already full

    // Merge textures into standard if possible
    if (test_flag(MergeFlags::MergeTextures))
    {
        const bool it_fits = textures->size().compressed + standard->size().compressed < standard->max_size();
        if (it_fits)
        {
            *standard += *textures;
            textures->clear();
        }
    }

    // Remove potentially empty archives
    std::erase_if(archives, [](const auto &arch) { return arch.empty(); });
}

} // namespace btu::bsa
