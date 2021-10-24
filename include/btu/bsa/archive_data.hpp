/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/detail/archive_type.hpp"
#include "btu/bsa/detail/common.hpp"
#include "btu/bsa/plugin.hpp"
#include "btu/bsa/settings.hpp"

#include <vector>

namespace btu::bsa {

class ArchiveData
{
public:
    ArchiveData(const Settings &sets, ArchiveType type);

    struct Size
    {
        uintmax_t compressed;
        uintmax_t uncompressed;

        auto operator<=>(const Size &) const = default;
        auto operator+=(const Size &other) -> Size &;
    };

    auto add_file(Path path, std::optional<Size> override = std::nullopt) -> bool;

    [[nodiscard]] auto get_type() const -> ArchiveType { return type_; }
    [[nodiscard]] auto get_version() const -> ArchiveVersion { return version_; }
    [[nodiscard]] auto empty() const -> bool;

    auto begin() { return files_.begin(); }
    auto end() { return files_.end(); }

    [[nodiscard]] auto find_name(const Path &folder, const Settings &sets) const -> Path;

    [[nodiscard]] auto size() const -> Size { return size_; }
    [[nodiscard]] auto max_size() const -> uintmax_t { return max_size_; }
    [[nodiscard]] auto type() const -> ArchiveType { return type_; }

    auto operator+=(const ArchiveData &other) -> ArchiveData &;
    auto operator+(ArchiveData const &other) const -> ArchiveData;

    auto operator<=>(const ArchiveData &) const = default;

    static constexpr bool merge_different_types    = true;
    static constexpr bool separate_different_types = false;

private:
    [[nodiscard]] static auto get_file_size(const common::Path &path, std::optional<Size> override) -> Size;

    Size size_{};
    uintmax_t max_size_ = UINT64_MAX;
    ArchiveType type_   = ArchiveType::Standard;
    ArchiveVersion version_{};
    std::vector<Path> files_;
};
} // namespace btu::bsa
