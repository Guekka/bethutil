/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/detail/archive_type.hpp"
#include "btu/bsa/plugin.hpp"
#include "btu/bsa/settings.hpp"
#include "btu/common/path.hpp"

#include <vector>

namespace btu::bsa {

class ArchiveData
{
public:
    ArchiveData(const Settings &sets, ArchiveType type, Path root_dir);

    auto add_file(const Path &absolute_path, std::optional<uint64_t> override = std::nullopt) -> bool;

    [[nodiscard]] auto get_type() const -> ArchiveType { return type_; }
    [[nodiscard]] auto get_version() const -> ArchiveVersion { return version_; }
    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto begin() noexcept { return files_.begin(); }
    [[nodiscard]] auto end() noexcept { return files_.end(); }

    [[nodiscard]] auto size() const -> uint64_t { return size_; }
    [[nodiscard]] auto max_size() const -> uint64_t { return max_size_; }
    [[nodiscard]] auto type() const -> ArchiveType { return type_; }

    auto operator+=(const ArchiveData &other) -> ArchiveData &;
    auto operator+(ArchiveData const &other) const -> ArchiveData;

    auto operator<=>(const ArchiveData &) const = default;

    [[nodiscard]] auto get_root_path() const noexcept -> Path;

private:
    uint64_t size_{};
    uint64_t max_size_ = UINT64_MAX;
    ArchiveType type_  = ArchiveType::Standard;
    ArchiveVersion version_{};

    Path root_dir_;
    std::vector<Path> files_;
};
} // namespace btu::bsa
