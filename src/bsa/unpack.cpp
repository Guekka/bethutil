/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/unpack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/functional.hpp"

#include <iostream>

namespace btu::bsa {
void unpack(UnpackSettings sets)
{
    {
        auto arch = Archive::read(sets.file_path);
        if (!arch)
            return;
        const auto &root = sets.root_opt != nullptr ? *sets.root_opt : sets.file_path.parent_path();
        btu::common::for_each_mt(std::move(*arch), [root, &sets](auto &&elem) {
            const auto path = root / elem.first;
            if (sets.overwrite_existing_files || !fs::exists(path)) // preserve existing loose files
            {
                fs::create_directories(path.parent_path());
                elem.second.write(path);
            }
        });
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
    std::ranges::for_each(files, [&out](const auto &file) {
        btu::bsa::unpack(UnpackSettings{.file_path = file.path(), .root_opt = &out});
    });
}

} // namespace btu::bsa
