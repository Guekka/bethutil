/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "string.hpp"

#include <filesystem>

namespace btu {
namespace fs   = std::filesystem;
using Path     = fs::path;
using OsString = Path::string_type;
using OsChar   = OsString::value_type;

namespace common {
constexpr auto make_path_canonizer(std::u8string_view start)
{
    return [start = std::move(start)](const Path &path) noexcept -> std::u8string {
        auto str        = btu::common::to_lower(path.generic_u8string());
        auto prefix_end = str.find(start);
        prefix_end      = prefix_end == std::string::npos ? 0 : prefix_end + start.size();
        return str.substr(prefix_end);
    };
}
} // namespace common
} // namespace btu
