/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp" 

#include "btu/bsa/detail/common.hpp"
#include "btu/bsa/detail/settings.hpp"
#include "btu/common/algorithms.hpp"

#include <deque>
#include <execution>
#include <fstream>
#include <functional>
#include <ranges>

namespace btu::bsa {
bool defaultIsAllowedPath(Path const &dir, fs::directory_entry const &fileinfo)
{
    bool const isDir = fileinfo.is_directory();

    //Removing files at the root directory, those cannot be packed
    bool const isRoot = fileinfo.path().parent_path() == dir;

    return !isDir && !isRoot;
}

void write(bool compressed, ArchiveData &&data, const Settings &sets, const Path &root)
{
    compressed &= data.get_type() != ArchiveType::Incompressible;

    auto arch = Archive(data.get_version(), compressed);
    std::for_each(data.begin(), data.end(), [&](auto &&f) { arch.add_file(root, f); });

    arch.write(data.find_name(root, sets));
}

void create(CreationSettings settings)
{
    const auto &dir  = settings.dir;
    const auto &sets = settings.settings;

    auto standard       = ArchiveData(sets, ArchiveType::Standard);
    auto incompressible = ArchiveData(sets, ArchiveType::Incompressible);
    auto textures       = ArchiveData(sets, ArchiveType::Textures);

    for (auto &p : fs::recursive_directory_iterator(dir))
    {
        if (!settings.allow_path_pred(dir, p))
            continue;

        auto const ft = get_filetype(p.path(), dir, sets);
        if (ft != FileTypes::Standard && ft != FileTypes::Texture && ft != FileTypes::Incompressible)
            continue;

        auto *pBSA = ft == FileTypes::Texture ? &textures : &standard;
        pBSA       = ft == FileTypes::Incompressible ? &incompressible : pBSA;

        //adding files and sizes to list
        if (!pBSA->add_file(p.path()))
        {
            // BSA full, write it
            write(settings.compress_archives, std::move(*pBSA), sets, dir);

            // Get a new one and add the file that did not make it
            *pBSA = ArchiveData(sets, pBSA->get_type());
            pBSA->add_file(p.path());
        }
    }

    // Here, we have at most three BSAs, that are all under the maximum size
    // Always merge textures into standard if possible
    std::vector<ArchiveData *> cleaned{&standard, &incompressible, &textures};
    if (textures.files_size() + standard.files_size() < standard.max_size())
    {
        standard += textures;
        cleaned.erase(cleaned.begin() + 2);
    }
    if (incompressible.files_size() + standard.files_size() < standard.max_size())
    {
        standard += incompressible;
        cleaned.erase(cleaned.begin() + 1);
    }

    for (auto *bsa : cleaned)
        write(settings.compress_archives, std::move(*bsa), sets, dir);
}

} // namespace btu::bsa
