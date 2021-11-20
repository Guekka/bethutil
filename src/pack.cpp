/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "btu/bsa/detail/backends/rsm_archive.hpp"
#include "btu/bsa/detail/common.hpp"
#include "btu/bsa/settings.hpp"

#include <btu/common/algorithms.hpp>
#include <btu/common/functional.hpp>

#include <deque>
#include <execution>
#include <fstream>
#include <functional>
#include <ranges>

namespace btu::bsa {
auto default_is_allowed_path(const Path &dir, fs::directory_entry const &fileinfo) -> bool
{
    bool const is_dir = fileinfo.is_directory();

    //Removing files at the root directory, those cannot be packed
    bool const is_root = fileinfo.path().parent_path() == dir;

    return !is_dir && !is_root;
}

auto write(bool compressed, ArchiveData &&data, const Settings &sets, const Path &root)
    -> std::vector<std::pair<Path, std::string>>
{
    if (std::distance(data.begin(), data.end()) == 0)
        return {}; // Do not write empty archive

    compressed &= data.get_type() != ArchiveType::Incompressible;

    auto arch = detail::RsmArchive(data.get_version(), compressed);
    auto ret  = std::vector<std::pair<Path, std::string>>();

    btu::common::for_each_mt(data, [&](auto &&f) {
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

auto split(const Path &dir, const Settings &sets, const AllowFilePred &allow_path_pred)
    -> std::vector<ArchiveData>
{
    auto standard       = ArchiveData(sets, ArchiveType::Standard);
    auto incompressible = ArchiveData(sets, ArchiveType::Incompressible);
    auto textures       = ArchiveData(sets, ArchiveType::Textures);

    std::vector<ArchiveData> res;

    for (const auto &p : fs::recursive_directory_iterator(dir))
    {
        if (!allow_path_pred(dir, p))
            continue;

        const auto ft = get_filetype(p.path(), dir, sets);
        if (ft != FileTypes::Standard && ft != FileTypes::Texture && ft != FileTypes::Incompressible)
            continue;

        auto *p_bsa = ft == FileTypes::Texture ? &textures : &standard;
        p_bsa       = ft == FileTypes::Incompressible ? &incompressible : p_bsa;

        //adding files and sizes to list
        if (!p_bsa->add_file(p.path()))
        {
            // BSA full, write it
            res.emplace_back(std::move(*p_bsa));

            // Get a new one and add the file that did not make it
            *p_bsa = ArchiveData(sets, p_bsa->get_type());
            p_bsa->add_file(p.path());
        }
    }

    // Add BSAs that are not full
    res.insert(res.end(), {std::move(standard), std::move(incompressible), std::move(textures)});
    return res;
}

auto merge(std::vector<ArchiveData> &archives, MergeSettings sets) -> void
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
