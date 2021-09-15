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
        Size &operator+=(const Size &other);
    };

    bool add_file(Path path, std::optional<Size> override = std::nullopt);

    ArchiveType get_type() const { return type_; }
    ArchiveVersion get_version() const { return version_; }
    bool empty() const;

    auto begin() { return files_.begin(); }
    auto end() { return files_.end(); }

    Path find_name(const Path &folder, const Settings &sets) const;

    Size size() const { return size_; }
    uintmax_t max_size() const { return max_size_; }
    ArchiveType type() const { return type_; }

    ArchiveData &operator+=(const ArchiveData &other);
    ArchiveData operator+(ArchiveData const &other) const;

    auto operator<=>(const ArchiveData &) const = default;

    static constexpr bool merge_different_types    = true;
    static constexpr bool separate_different_types = false;

private:
    static constexpr bool k_acurate_size_estimation = false;

    Size get_file_size(const common::Path &path, std::optional<Size> override) const;

    Size size_{};
    uintmax_t max_size_ = -1;
    ArchiveType type_   = ArchiveType::Standard;
    ArchiveVersion version_{};
    std::vector<Path> files_;
};
} // namespace btu::bsa
