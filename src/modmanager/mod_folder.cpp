/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/filesystem.hpp"

namespace btu::modmanager {
detail::ModFileDisk::ModFileDisk(Path root, Path rel_path)
    : relative_path_(std::move(rel_path))
    , root_(std::move(root))
{
}

void detail::ModFileDisk::load()
{
    read(root_ / relative_path_);
    modified_ = false;
}

auto detail::ModFileDisk::get_relative_path() const noexcept -> Path
{
    return relative_path_;
}

auto detail::ModFileDisk::size() const noexcept -> size_t
{
    return content_.size();
}

auto detail::ModFileDisk::modified() const noexcept -> bool
{
    return modified_;
}

void detail::ModFileDisk::read(const Path &path)
{
    modified_ = true;
    content_  = btu::common::read_file(path);
}

void detail::ModFileDisk::read(std::span<std::byte> src)
{
    modified_ = true;
    content_.assign(src.begin(), src.end());
}

void detail::ModFileDisk::write(const Path &path) const
{
    btu::common::write_file(path, content_);
}

void detail::ModFileDisk::write(binary_io::any_ostream &dst) const
{
    dst.write_bytes(content_);
}

ModFile::ModFile(Underlying content)
    : file_(std::move(content))
{
}

void ModFile::load()
{
    if (auto *f = std::get_if<std::reference_wrapper<detail::ModFileDisk>>(&file_))
        f->get().load();
}

auto ModFile::get_relative_path() const -> Path
{
    auto visitor = btu::common::overload{
        [](const detail::ModFileArchive &f) -> Path { return f.name; },
        [](const detail::ModFileDisk &f) { return f.get_relative_path(); },
    };

    return std::visit(visitor, file_);
}

void ModFile::read(Path path)
{
    auto visitor = btu::common::overload{
        [&](detail::ModFileArchive &f) { f.file.read(std::move(path)); },
        [&](detail::ModFileDisk &f) { f.read(path); },
    };

    return std::visit(visitor, file_);
}

void ModFile::read(std::span<std::byte> src)
{
    auto visitor = btu::common::overload{
        [src](detail::ModFileArchive &f) { f.file.read(src); },
        [src](detail::ModFileDisk &f) { f.read(src); }, // TODO
    };

    return std::visit(visitor, file_);
}

void ModFile::write(const Path &path) const
{
    auto visitor = btu::common::overload{
        [&path](const detail::ModFileArchive &f) { f.file.write(path); },
        [&path](const detail::ModFileDisk &f) { f.write(path); },
    };

    return std::visit(visitor, file_);
}

void ModFile::write(binary_io::any_ostream &dst) const
{
    auto visitor = btu::common::overload{
        [&](const detail::ModFileArchive &f) { f.file.write(dst); },
        [&](const detail::ModFileDisk &f) { f.write(dst); },
    };

    return std::visit(visitor, file_);
}

ModFolder::ModFolder(Path directory, std::u8string_view archive_ext)
    : dir_(std::move(directory))
    , archive_ext_(btu::common::to_lower(archive_ext))
{
    //Typical amount of medium mod
    archives_.reserve(4);
    loose_files_.reserve(1'000);

    auto is_arch = [this](const Path &file_name) {
        auto ext = btu::common::to_lower(file_name.extension().u8string());
        return ext == archive_ext_;
    };

    flow::from(fs::recursive_directory_iterator(dir_))
        .filter([](auto &&e) { return e.is_regular_file(); })
        .map([](auto &&e) { return e.path(); })
        .for_each([&is_arch, this](auto &&path) {
            if (is_arch(path))
            {
                if (auto arch = btu::bsa::read_archive(path))
                    archives_.emplace_back(Archive{std::forward<decltype(path)>(path), *std::move(arch)});
            }
            else
            {
                const auto rel = path.lexically_relative(dir_);
                loose_files_.emplace_back(dir_, rel);
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

void ModFolder::reintegrate()
{
    for (const auto &arch : archives_)
        bsa::write_archive(arch.arch, arch.path);
    flow::from(loose_files_).filter(&detail::ModFileDisk::modified).for_each([this](auto &&f) {
        f.write(dir_ / f.get_relative_path());
    });
}

} // namespace btu::modmanager
