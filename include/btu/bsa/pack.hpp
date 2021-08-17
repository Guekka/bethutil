/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/detail/archive_data.hpp"
#include "btu/bsa/detail/common.hpp"

namespace btu::bsa {
using AllowFilePred = std::function<bool(const Path &dir, fs::directory_entry const &fileinfo)>;

bool defaultIsAllowedPath(Path const &dir, fs::directory_entry const &fileinfo);

struct CreationSettings
{
    const Path &dir;
    bool compact_archives;
    bool compress_archives;
    const Settings &settings;
    AllowFilePred allow_path_pred = defaultIsAllowedPath;
};

void create(CreationSettings sets);

} // namespace btu::bsa
