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

namespace common {
/// @brief Convert backslashes to slashes in path.
/// @details std::fs do not consider '\' as a separator on Linux. In fact, it is a valid character in file names.
/// But in our case, we want to treat it as a separator. Let's hope no one will use it in file names.
template<typename CharT>
void backslash_to_slash(std::basic_string<CharT> &path) noexcept
{
    std::ranges::replace(path, static_cast<CharT>('\\'), static_cast<CharT>('/'));
}

constexpr auto make_path_canonizer(std::u8string_view start)
{
    return [start](const Path &path) noexcept -> std::u8string {
        auto str = btu::common::to_lower(path.generic_u8string());
        backslash_to_slash(str);

        auto trimmed = str_trim(str);

        auto prefix_end = trimmed.find(start);
        prefix_end      = prefix_end == std::string::npos ? 0 : prefix_end + start.size();
        return std::u8string(trimmed.substr(prefix_end));
    };
}
} // namespace common
} // namespace btu
