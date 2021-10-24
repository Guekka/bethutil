/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "string.hpp"

#include <filesystem>

namespace btu::common {
using Path     = std::filesystem::path;
using OsString = Path::string_type;
using OsChar   = OsString::value_type;
} // namespace btu::common
