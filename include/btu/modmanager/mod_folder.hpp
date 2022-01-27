/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/bsa/archive.hpp"
#include "btu/common/path.hpp"

#include <flow.hpp>

#include <variant>

namespace btu::modmanager {
using btu::common::Path;

namespace detail {
struct ModFileDisk
{
    Path file_path;
    Path relative_path;
};

struct ModFileArchive
{
    std::string name;
    btu::bsa::File &file;
};

} // namespace detail

class ModFile
{
public:
    ModFile(std::variant<detail::ModFileArchive, detail::ModFileDisk> content);

    auto get_relative_path() const -> Path;

    void read(std::filesystem::path path);
    void read(std::span<std::byte> src);

    void write(std::filesystem::path path) const;
    void write(binary_io::any_ostream &dst) const;

private:
    std::variant<detail::ModFileArchive, detail::ModFileDisk> file_;
};

class ModFolder
{
public:
    explicit ModFolder(Path directory, std::u8string archive_ext);

    [[nodiscard]] auto size() -> size_t;

    [[nodiscard]] auto to_flow()
    {
        return flow::from(archives_)
            .flatten()
            .map([](auto &pair) {
                return ModFile(detail::ModFileArchive{pair.first, pair.second});
            })
            .chain(flow::copy(loose_files_));
    }

private:
    std::vector<btu::bsa::Archive> archives_;
    std::vector<ModFile> loose_files_;

    Path dir_;
    std::u8string archive_ext_;
};
} // namespace btu::modmanager
