/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#define _SILENCE_CLANG_COROUTINE_MESSAGE
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
};

struct ModFileArchive
{
    Path rel_path;
    btu::bsa::File file;
};

} // namespace detail

class ModFile
{
public:
    ModFile(std::variant<detail::ModFileArchive, detail::ModFileDisk> content)
        : file_(std::move(content))
    {
    }

    auto is_on_disk() -> bool;
    auto get_or_write(const Path *out) -> const Path &;

    auto &get_underlying() { return file_; }

private:
    std::variant<detail::ModFileArchive, detail::ModFileDisk> file_;
};

class ModFolder
{
public:
    explicit ModFolder(Path directory, std::u8string archive_ext);

    [[nodiscard]] auto size() -> size_t;

    [[nodiscard]] auto as_flow() const
    {
        return flow::from(archives_)
            .flat_map([](auto &a) { return a->to_flow(); })
            .map([](const auto &pair) {
                return ModFile(detail::ModFileArchive{pair.first, pair.second});
            })
            .chain(flow::copy(loose_files_));
    }

private:
    std::vector<std::unique_ptr<btu::bsa::Archive>> archives_;
    std::vector<ModFile> loose_files_;

    size_t count_;

    Path dir_;
    std::u8string archive_ext_;
};
} // namespace btu::modmanager
