/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/archive_data.hpp"
#include "btu/bsa/detail/common.hpp"

namespace btu::bsa {
using AllowFilePred = std::function<bool(const Path &dir, fs::directory_entry const &fileinfo)>;

auto default_is_allowed_path(const Path &dir, fs::directory_entry const &fileinfo) -> bool;

enum class MergeSettings : std::uint8_t
{
    MergeTextures       = 1,
    MergeIncompressible = 1 << 1,
};

BETHUTIL_MAKE_ALL_ENUM_OPERATORS(MergeSettings);

auto split(const Path &dir, const Settings &sets, AllowFilePred allow_path_pred = default_is_allowed_path)
    -> std::vector<ArchiveData>;

void merge(std::vector<ArchiveData> &archives,
           MergeSettings sets = MergeSettings::MergeIncompressible | MergeSettings::MergeTextures);

/// Returns the list of files which failed to pack
auto write(bool compressed, ArchiveData &&data, const Settings &sets, const Path &root)
    -> std::vector<std::pair<Path, std::string>>;

} // namespace btu::bsa
