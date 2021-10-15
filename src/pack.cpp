/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "btu/bsa/detail/backends/rsm_archive.hpp"
#include "btu/bsa/detail/common.hpp"
#include "btu/bsa/settings.hpp"
#include "btu/common/algorithms.hpp"

#include <deque>
#include <execution>
#include <fstream>
#include <functional>
#include <ranges>

namespace btu::bsa {
bool default_is_allowed_path(const Path &dir, fs::directory_entry const &fileinfo)
{
    bool const is_dir = fileinfo.is_directory();

    //Removing files at the root directory, those cannot be packed
    bool const is_root = fileinfo.path().parent_path() == dir;

    return !is_dir && !is_root;
}

std::vector<std::pair<Path, std::string>> write(bool compressed,
                                                ArchiveData &&data,
                                                const Settings &sets,
                                                const Path &root)
{
    if (std::distance(data.begin(), data.end()) == 0)
        return {}; // Do not write empty archive

    compressed &= data.get_type() != ArchiveType::Incompressible;

    auto arch = detail::RsmArchive(data.get_version(), compressed);
    auto ret  = std::vector<std::pair<Path, std::string>>();

    std::for_each(std::execution::par, data.begin(), data.end(), [&](auto &&f) {
        try
        {
            arch.add_file(root, f);
        }
        catch (const std::exception &e)
        {
            ret.emplace_back(f, e.what());
        }
    });

    arch.write(data.find_name(root, sets));
    return ret;
}

std::vector<ArchiveData> split(const Path &dir, const Settings &sets, AllowFilePred allow_path_pred)
{
    auto standard       = ArchiveData(sets, ArchiveType::Standard);
    auto incompressible = ArchiveData(sets, ArchiveType::Incompressible);
    auto textures       = ArchiveData(sets, ArchiveType::Textures);

    std::vector<ArchiveData> res;

    for (auto &p : fs::recursive_directory_iterator(dir))
    {
        if (!allow_path_pred(dir, p))
            continue;

        const auto ft = get_filetype(p.path(), dir, sets);
        if (ft != FileTypes::Standard && ft != FileTypes::Texture && ft != FileTypes::Incompressible)
            continue;

        auto *pBSA = ft == FileTypes::Texture ? &textures : &standard;
        pBSA       = ft == FileTypes::Incompressible ? &incompressible : pBSA;

        //adding files and sizes to list
        if (!pBSA->add_file(p.path()))
        {
            // BSA full, write it
            res.emplace_back(std::move(*pBSA));

            // Get a new one and add the file that did not make it
            *pBSA = ArchiveData(sets, pBSA->get_type());
            pBSA->add_file(p.path());
        }
    }

    // Add BSAs that are not full
    res.insert(res.end(), {std::move(standard), std::move(incompressible), std::move(textures)});
    return res;
}

void merge(std::vector<ArchiveData> &archives, MergeSettings sets)
{
    const auto test_flag = [&](MergeSettings flag) {
        return (btu::common::to_underlying(sets) & btu::common::to_underlying(flag)) != 0;
    };
    // We have at most one underful BSA per type, so we only consider the last three BSAs

    auto standard       = archives.end() - 3;
    auto incompressible = archives.end() - 2;
    auto textures       = archives.end() - 1;

    // Preconditions
    assert(standard->get_type() == ArchiveType::Standard);
    assert(incompressible->get_type() == ArchiveType::Incompressible);
    assert(textures->get_type() == ArchiveType::Textures);

    // Merge textures into standard if possible
    if (test_flag(MergeSettings::MergeTextures))
    {
        if (textures->size().compressed + standard->size().compressed < standard->max_size())
        {
            *standard += *textures;
            archives.erase(textures);
        }
    }

    if (test_flag(MergeSettings::MergeIncompressible))
    {
        if (incompressible->size().uncompressed + standard->size().uncompressed < standard->max_size())
        {
            *standard += *incompressible;
            archives.erase(incompressible);
        }
    }
}

} // namespace btu::bsa
