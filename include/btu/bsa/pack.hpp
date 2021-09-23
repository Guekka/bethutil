/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/archive_data.hpp"
#include "btu/bsa/detail/common.hpp"

namespace btu::bsa {
using AllowFilePred = std::function<bool(const Path &dir, fs::directory_entry const &fileinfo)>;

bool defaultIsAllowedPath(const Path &dir, fs::directory_entry const &fileinfo);

enum MergeSettings
{
    MergeTextures,
    MergeIncompressible,
    MergeBoth
};

std::vector<ArchiveData> split(const Path &dir,
                               const Settings &sets,
                               AllowFilePred allow_path_pred = defaultIsAllowedPath);

void merge(std::vector<ArchiveData> &archives, MergeSettings sets = MergeBoth);

/// Returns the list of files which failed to pack
std::vector<std::pair<Path, std::string>> write(bool compressed,
                                                ArchiveData &&data,
                                                const Settings &sets,
                                                const Path &root);

} // namespace btu::bsa
