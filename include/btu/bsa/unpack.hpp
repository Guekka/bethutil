/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/detail/backends/rsm_archive.hpp"
#include "btu/bsa/settings.hpp"

#include <fstream>
#include <iostream>
#include <mutex>

namespace btu::bsa {
[[nodiscard]] inline auto open_virtual_path(const Path &path) -> std::ofstream
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out{path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc};
    out.exceptions(std::ios_base::failbit);
    return out;
}

struct UnpackSettings
{
    const Path &file_path;
    bool remove_arch              = false;
    bool overwrite_existing_files = false;
    const Path *root_opt          = nullptr;
};

inline void unpack(UnpackSettings sets)
{
    auto arch = detail::RsmArchive(sets.file_path);

    const auto &root = sets.root_opt ? *sets.root_opt : sets.file_path.parent_path();
    arch.iterate_files([&](const fs::path &rel, std::span<const std::byte> data) {
        auto raw_out = [&]() -> std::optional<std::ofstream> {
            std::mutex mut;
            std::scoped_lock lock(mut);
            const auto path = root / rel;
            if (fs::exists(path) && !sets.overwrite_existing_files)
                return std::nullopt;

            return open_virtual_path(path);
        }();

        if (!raw_out.has_value())
            return;
        auto &out = *raw_out;

        out.write(reinterpret_cast<const char *>(data.data()), data.size());
    });

    if (sets.remove_arch && !fs::remove(sets.file_path))
    {
        throw std::runtime_error("BSA Extract succeeded but failed to delete the extracted BSA");
    }
}

inline void unpack_all(const Path &dir, const Path &out, const Settings &sets)
{
    std::vector files(fs::directory_iterator(dir), fs::directory_iterator{});
    erase_if(files, [&sets](const auto &file) { return file.path().extension() != sets.extension; });
    std::for_each(files.begin(), files.end(), [&out](const auto &file) {
        btu::bsa::unpack(UnpackSettings{.file_path = file.path(), .root_opt = &out});
    });
}
} // namespace btu::bsa
