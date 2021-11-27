/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/unpack.hpp"

#include "btu/bsa/detail/backends/rsm_archive.hpp"

#include <fstream>
#include <iostream>
#include <mutex>

namespace btu::bsa {
void unpack(UnpackSettings sets)
{
    {
        auto arch        = detail::RsmArchive(sets.file_path);
        const auto &root = sets.root_opt ? *sets.root_opt : sets.file_path.parent_path();
        arch.unpack(root);
    }
    if (sets.remove_arch && !fs::remove(sets.file_path))
    {
        throw std::runtime_error("BSA Extract succeeded but failed to delete the extracted BSA");
    }
}

void unpack_all(const Path &dir, const Path &out, const Settings &sets)
{
    std::vector files(fs::directory_iterator(dir), fs::directory_iterator{});
    erase_if(files, [&sets](const auto &file) { return file.path().extension() != sets.extension; });
    std::for_each(files.begin(), files.end(), [&out](const auto &file) {
        btu::bsa::unpack(UnpackSettings{.file_path = file.path(), .root_opt = &out});
    });
}

} // namespace btu::bsa
