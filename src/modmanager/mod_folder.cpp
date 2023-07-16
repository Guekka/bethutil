/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/memory_stream.hpp>

#include <atomic>
#include <utility>

namespace btu::modmanager {

ModFolder::ModFolder(Path directory, std::u8string archive_ext)
    : dir_(std::move(directory))
    , archive_ext_(std::move(archive_ext))
{
}
void ModFolder::iterate(
    const std::function<void(Path relative_path)> &loose,
    const std::function<void(const Path &archive_path, bsa::Archive &&archive)> &archive) const
{
    auto is_arch = [this](const Path &file_name) {
        auto ext = btu::common::to_lower(file_name.extension().u8string());
        return ext == archive_ext_;
    };

    flow::from(fs::recursive_directory_iterator(dir_))
        .filter([](auto &&e) { return e.is_regular_file(); })
        .map([](auto &&e) { return e.path(); })
        .for_each([&is_arch, this, &archive, &loose](auto &&path) {
            if (!is_arch(path)) [[likely]]
            {
                loose(path.lexically_relative(dir_));
                return;
            }

            if (auto arch = btu::bsa::read_archive(path))
                archive(path, std::move(*arch));
        });
}

auto ModFolder::size() const -> size_t
{
    std::atomic<size_t> size = 0;
    iterate([&size](const Path &) { size += 1; },
            [&size](const Path &, bsa::Archive &&archive) { size += archive.size(); });

    return size;
}

void ModFolder::iterate(const std::function<void(ModFile)> &visitor) const
{
    transform_impl([&visitor](ModFile file) {
        visitor(std::move(file));
        return std::nullopt;
    });
}

void ModFolder::transform(ModFolder::Transformer &&transformer)
{
    transform_impl(std::move(transformer));
}

void ModFolder::transform_impl(ModFolder::Transformer &&transformer) const
{
    iterate(
        [this, &transformer](const Path &relative_path) {
            auto file_data   = common::read_file(dir_ / relative_path);
            auto transformed = transformer({relative_path, file_data});
            if (transformed)
                common::write_file(dir_ / relative_path, *transformed);
        },
        [&transformer](const Path &archive_path, bsa::Archive &&archive) {
            bool any_file_changed = false;

            for (auto &&[relative_path, file] : archive)
            {
                auto buffer = binary_io::any_ostream{binary_io::memory_ostream{}};
                file.write(buffer);

                auto &file_data = buffer.get<binary_io::memory_ostream>().rdbuf();

                auto transformed = transformer({relative_path, file_data});

                if (transformed)
                {
                    file.read(*transformed);
                    any_file_changed = true;
                }
            }

            if (any_file_changed)
                write_archive(std::move(archive), archive_path);
        });
}

} // namespace btu::modmanager
