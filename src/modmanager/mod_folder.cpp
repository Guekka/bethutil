/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/file_stream.hpp>

namespace btu::modmanager {
ModFile::ModFile(std::variant<detail::ModFileArchive, detail::ModFileDisk> content)
    : file_(std::move(content))
{
}

auto ModFile::get_relative_path() const -> Path
{
    auto visitor = btu::common::overload{
        [](const detail::ModFileArchive &f) -> Path { return f.name; },
        [](const detail::ModFileDisk &f) { return f.relative_path; },
    };

    return std::visit(visitor, file_);
}

void ModFile::read(std::filesystem::path path)
{
    auto visitor = btu::common::overload{
        [&](detail::ModFileArchive &f) { f.file.read(std::move(path)); },
        [&](detail::ModFileDisk &f) { f.file_path = std::move(path); },
    };

    return std::visit(visitor, file_);
}

void ModFile::read(std::span<std::byte> src)
{
    auto visitor = btu::common::overload{
        [&](detail::ModFileArchive &f) { f.file.read(src); },
        [&](detail::ModFileDisk &) { assert(false); }, // TODO
    };

    return std::visit(visitor, file_);
}

void ModFile::write(std::filesystem::path path) const
{
    auto visitor = btu::common::overload{
        [&](const detail::ModFileArchive &f) { f.file.write(std::move(path)); },
        [&](const detail::ModFileDisk &f) { std::filesystem::copy(f.file_path, std::move(path)); },
    };

    return std::visit(visitor, file_);
}

void ModFile::write(binary_io::any_ostream &dst) const
{
    auto visitor = btu::common::overload{
        [&](const detail::ModFileArchive &f) { f.file.write(dst); },
        [&](const detail::ModFileDisk &f) { dst.write_bytes(btu::common::read_file(f.file_path)); },
    };

    return std::visit(visitor, file_);
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

    flow::from(std::filesystem::recursive_directory_iterator(dir_))
        .filter([](auto &&e) { return e.is_regular_file(); })
        .map([](auto &&e) { return e.path(); })
        .for_each([&](auto &&path) {
            if (is_arch(path))
            {
                if (auto arch = btu::bsa::read_archive(path))
                    archives_.emplace_back(*std::move(arch));
            }
            else
            {
                const auto rel = path.lexically_relative(dir_);
                loose_files_.emplace_back(detail::ModFileDisk{std::move(path), std::move(rel)});
            }
        });

    //Memory optimization
    archives_.shrink_to_fit();
    loose_files_.shrink_to_fit();
}

auto ModFolder::size() -> size_t
{
    return to_flow().count();
}

} // namespace btu::modmanager
