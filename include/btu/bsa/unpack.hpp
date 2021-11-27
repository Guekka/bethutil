/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/settings.hpp"

namespace btu::bsa {
[[nodiscard]] auto open_virtual_path(const Path &path) -> std::ofstream;

struct UnpackSettings
{
    const Path &file_path;
    bool remove_arch              = false;
    bool overwrite_existing_files = false;
    const Path *root_opt          = nullptr;
};

void unpack(UnpackSettings sets);

inline void unpack_all(const Path &dir, const Path &out, const Settings &sets);
} // namespace btu::bsa
