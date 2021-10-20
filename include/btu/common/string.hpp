/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/metaprogramming.hpp"

#include <cctype>
#include <string>

namespace btu::common {
using u8_unit = int32_t;

class InvalidUTF8 : public std::exception
{
public:
    InvalidUTF8(const void *location)
        : location(location)
    {
    }

    const void *location;
    const char *what() const override { return "Invalid UTF8 string given in argument"; }
};

std::u8string_view as_utf8(std::string_view str);

auto str_compare(std::u8string_view lhs, std::u8string_view rhs, bool case_sensitive = true) -> bool;
auto str_find(std::u8string_view string, std::u8string_view snippet, bool case_sensitive = true) -> size_t;
auto str_contain(std::u8string_view string, std::u8string_view snippet, bool caseSensitive = true) -> bool;

auto to_lower(std::u8string_view str) -> std::u8string;
auto to_lower(std::u8string &&str) -> std::u8string;

} // namespace btu::common
