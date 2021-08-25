/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/detail/archive.hpp"
#include "btu/bsa/detail/settings.hpp"

#include <fstream>

namespace btu::bsa {
[[nodiscard]] auto open_virtual_path(const Path &path) -> std::ofstream
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out{path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc};
    out.exceptions(std::ios_base::failbit);
    return out;
}

inline void unpack(const Path &file_path, bool remove_arch = false, bool overwrite_existing_files = false)
{
    Archive arch(file_path);
    const auto root = file_path.parent_path();
    arch.iterate_files(
        [&root, overwrite_existing_files](const fs::path &rel, std::span<const std::byte> data) {
            const auto path = root / rel;
            if (fs::exists(path) && !overwrite_existing_files)
                return;

            auto out = open_virtual_path(path);
            out.write(reinterpret_cast<const char *>(data.data()), data.size());
        });

    if (remove_arch && !fs::remove(file_path))
    {
        throw std::runtime_error("BSA Extract succeeded but failed to delete the extracted BSA");
    }
}

inline void unpack_all(const Path &dir, const Settings &sets)
{
    std::vector files(fs::directory_iterator(dir), fs::directory_iterator{});
    erase_if(files, [&sets](const auto &file) { return file.path().extension() != sets.extension; });
    std::for_each(files.begin(), files.end(), [](const auto &file) { btu::bsa::unpack(file.path()); });
}
} // namespace btu::bsa
