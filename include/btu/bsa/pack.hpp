/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/archive_data.hpp"

namespace btu::bsa {
using AllowFilePred = std::function<bool(const Path &dir, fs::directory_entry const &fileinfo)>;

auto default_is_allowed_path(const Path &dir, fs::directory_entry const &fileinfo) -> bool;

auto prepare_archive(const Path &dir,
                     const Settings &sets,
                     const AllowFilePred &allow_path_pred = default_is_allowed_path)
    -> std::vector<ArchiveData>;

/// \return the list of files which failed to pack
auto write(Path filepath, Compression compressed, ArchiveData &&data)
    -> std::vector<std::pair<Path, std::string>>;

} // namespace btu::bsa
