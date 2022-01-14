/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "btu/bsa/archive.hpp"

#include <binary_io/file_stream.hpp>

namespace btu::modmanager {
auto ModFile::is_on_disk() -> bool
{
    return std::holds_alternative<detail::ModFileDisk>(file_);
}

auto ModFile::get_or_write(const Path *out) -> const Path &
{
    if (auto *fd = std::get_if<detail::ModFileDisk>(&file_))
    {
        return fd->file_path;
    }
    else if (auto *it = std::get_if<detail::ModFileArchive>(&file_))
    {
        assert(out);
        binary_io::any_ostream stream{std::in_place_type<binary_io::file_ostream>, *out};
        it->file.write(stream);
        file_ = detail::ModFileDisk{*out};
        return std::get<detail::ModFileDisk>(file_).file_path;
    }
    libbsa::detail::declare_unreachable();
}

ModFolder::ModFolder(Path directory, std::u8string archive_ext)
    : dir_(std::move(directory))
    , archive_ext_(btu::common::to_lower(std::move(archive_ext)))
{
    //Typical amount of medium mod
    archives_.reserve(4);
    loose_files_.reserve(1'000);

    auto is_arch = [this](const Path &file_name) {
        auto ext = btu::common::to_lower(file_name.extension().u8string());
        return ext == archive_ext_;
    };

    for (auto &&f : std::filesystem::recursive_directory_iterator(dir_))
    {
        if (!f.is_regular_file())
            return;
        const auto path = f.path();
        if (is_arch(path))
        {
            auto arch = std::make_unique<btu::bsa::Archive>(path);
            count_ += arch->file_count();
            archives_.emplace_back(std::move(arch));
        }
        else
        {
            loose_files_.emplace_back(detail::ModFileDisk{std::move(path)});
        }
    }

    //Memory optimization
    archives_.shrink_to_fit();
    loose_files_.shrink_to_fit();
}

} // namespace btu::modmanager
