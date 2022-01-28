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
class ModFileDisk final
{
public:
    ModFileDisk(Path root, Path rel_path);

    void load();

    auto get_relative_path() const noexcept -> Path;
    [[nodiscard]] auto size() const noexcept -> size_t;

    [[nodiscard]] auto modified() const noexcept -> bool;

    void read(const std::filesystem::path &path);
    void read(std::span<std::byte> src);

    void write(const std::filesystem::path &path) const;
    void write(binary_io::any_ostream &dst) const;

private:
    std::vector<std::byte> content_;
    Path relative_path_;
    Path root_;

    bool modified_{};
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
    using Underlying = std::variant<detail::ModFileArchive, std::reference_wrapper<detail::ModFileDisk>>;

    ModFile(Underlying content);

    void load();

    auto get_relative_path() const -> Path;

    void read(std::filesystem::path path);
    void read(std::span<std::byte> src);

    void write(std::filesystem::path path) const;
    void write(binary_io::any_ostream &dst) const;

    const auto &get_underlying() const { return file_; }

private:
    Underlying file_;
};

class ModFolder
{
    struct Archive
    {
        Path path;
        btu::bsa::Archive arch;
    };

public:
    explicit ModFolder(Path directory, std::u8string archive_ext);

    [[nodiscard]] auto size() -> size_t;

    [[nodiscard]] auto to_flow()
    {
        return flow::map(archives_, &Archive::arch)
            .flatten()
            .map([](auto &pair) {
                return ModFile(detail::ModFileArchive{pair.first, pair.second});
            })
            .chain(flow::map(loose_files_, [](auto &f) { return ModFile(std::ref(f)); }));
    }

    void reintegrate();

private:
    std::vector<Archive> archives_;
    std::vector<detail::ModFileDisk> loose_files_;

    Path dir_;
    std::u8string archive_ext_;
};
} // namespace btu::modmanager
