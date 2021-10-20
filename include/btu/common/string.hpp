/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/metaprogramming.hpp"

#include <cctype>
#include <string>

namespace btu::common {
using U8Unit = int32_t;

class UTF8Iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = U8Unit;
    using pointer           = U8Unit;
    using reference         = U8Unit;

    explicit UTF8Iterator(std::u8string_view string);
    static auto end(std::u8string_view string) -> UTF8Iterator;

    auto operator*() const -> reference;
    auto operator->() const -> pointer;

    // Prefix increment
    auto operator++() -> UTF8Iterator &;

    // Postfix increment
    auto operator++(int) -> UTF8Iterator;

    auto operator<=>(const UTF8Iterator &) const = default;

private:
    std::u8string_view string_;
    size_t idx_{};
    value_type cur_{};
};

class InvalidUTF8 : public std::exception
{
public:
    [[nodiscard]] auto what() const -> const char * override
    {
        return "Invalid UTF8 string given in argument";
    }
};

auto as_utf8(std::string_view str) -> std::u8string_view;
auto as_ascii(std::u8string_view str) -> std::string_view;

auto str_compare(std::u8string_view lhs, std::u8string_view rhs, bool case_sensitive = true) -> bool;
auto str_find(std::u8string_view string, std::u8string_view snippet, bool case_sensitive = true) -> size_t;
auto str_contain(std::u8string_view string, std::u8string_view snippet, bool case_sensitive = true) -> bool;

auto to_lower(std::u8string_view string) -> std::u8string;

auto first_codepoint(std::u8string_view string) -> U8Unit;
auto concat_codepoint(std::u8string &string, U8Unit cp) -> void;

} // namespace btu::common
