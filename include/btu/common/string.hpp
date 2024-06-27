/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/metaprogramming.hpp"

#include <utf8h/utf8.h>

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <cstdint>
#include <ranges>
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

    constexpr explicit UTF8Iterator(std::u8string_view string);
    [[nodiscard]] constexpr static auto end(std::u8string_view string) -> UTF8Iterator;

    [[nodiscard]] constexpr auto operator*() const -> reference;
    [[nodiscard]] constexpr auto operator->() const -> pointer;

    // Prefix increment
    constexpr auto operator++() -> UTF8Iterator &;

    // Postfix increment
    constexpr auto operator++(int) -> UTF8Iterator;

    [[nodiscard]] constexpr auto operator<=>(const UTF8Iterator &other) const -> std::strong_ordering;
    [[nodiscard]] constexpr auto operator==(const UTF8Iterator &other) const -> bool;

private:
    std::u8string_view string_;
    size_t idx_{};
    value_type cur_{};
};

class InvalidUTF8 final : public std::exception
{
public:
    [[nodiscard]] constexpr auto what() const noexcept -> const char * override
    {
        return "Invalid UTF8 string given in argument";
    }
};

enum class CaseSensitive : std::uint8_t
{
    Yes,
    No
};

auto as_utf8(std::string_view str) -> std::u8string_view;
auto as_ascii(std::u8string_view str) -> std::string_view;

auto as_utf8_string(std::string str) -> std::u8string;
auto as_ascii_string(std::u8string str) -> std::string;

auto to_utf8(const std::wstring &str) -> std::u8string;
auto to_utf16(const std::u8string &str) -> std::wstring;

constexpr auto str_compare(std::u8string_view lhs,
                           std::u8string_view rhs,
                           CaseSensitive case_sensitive = CaseSensitive::Yes) -> bool;

constexpr auto str_find(std::u8string_view string,
                        std::u8string_view snippet,
                        CaseSensitive case_sensitive = CaseSensitive::Yes) -> size_t;
constexpr auto str_contain(std::u8string_view string,
                           std::u8string_view snippet,
                           CaseSensitive case_sensitive = CaseSensitive::Yes) -> bool;
[[nodiscard]] constexpr auto str_starts_with(std::u8string_view string,
                                             std::u8string_view snippet,
                                             CaseSensitive case_sensitive = CaseSensitive::Yes) -> bool;

/* Returns a string_view of the string without leading and trailing whitespace, including null */
[[nodiscard]] constexpr auto str_trim(std::u8string_view in) noexcept -> std::u8string_view;

struct Cards
{
    U8Unit any;
    U8Unit any_repeat;
    U8Unit set_begin;
    U8Unit set_end;
};

constexpr Cards default_cards{u8'?', u8'*', u8'[', u8']'};

constexpr auto str_match(std::u8string_view string,
                         std::u8string_view pattern,
                         CaseSensitive case_sensitive = CaseSensitive::Yes,
                         Cards cards                  = default_cards) -> bool;

auto to_lower(std::u8string_view string) -> std::u8string;
constexpr auto is_lower(std::u8string_view string) -> bool;

constexpr auto first_codepoint(std::u8string_view string) -> U8Unit;
void concat_codepoint(std::u8string &string, U8Unit cp);
auto make_valid(std::u8string &string, char8_t replacement) noexcept -> std::u8string &;

namespace detail {

constexpr void assert_valid_utf8([[maybe_unused]] std::u8string_view str)
{
#ifndef NDEBUG
    if (utf8nvalid(str.data(), str.size()) != nullptr)
        throw InvalidUTF8{};
#endif
}
} // namespace detail

constexpr UTF8Iterator::UTF8Iterator(std::u8string_view string)
    : string_(string)
{
    if (string_.empty())
        return;

    detail::assert_valid_utf8(string_);
    utf8codepoint(string_.data(), &cur_);
}

constexpr auto UTF8Iterator::end(std::u8string_view string) -> UTF8Iterator
{
    UTF8Iterator it(string);
    while (it.idx_ < it.string_.size())
        ++it;
    return it;
}

constexpr auto UTF8Iterator::operator*() const -> value_type
{
    return cur_;
}

constexpr auto UTF8Iterator::operator->() const -> pointer
{
    return cur_;
}

constexpr auto UTF8Iterator::operator++() -> UTF8Iterator &
{
    idx_ += utf8codepointsize(cur_);
    utf8codepoint(string_.data() + idx_, &cur_);
    return *this;
}

constexpr auto UTF8Iterator::operator<=>(const UTF8Iterator &other) const -> std::strong_ordering
{
    if (idx_ != other.idx_)
        return idx_ <=> other.idx_;

    if (cur_ != other.cur_)
        return cur_ <=> other.cur_;

    return string_.data() <=> other.string_.data();
}

constexpr auto UTF8Iterator::operator++(int) -> UTF8Iterator
{
    const auto copy = *this;
    ++*this;
    return copy;
}
constexpr auto UTF8Iterator::operator==(const UTF8Iterator &other) const -> bool
{
    return *this <=> other == std::strong_ordering::equal;
}

static_assert(sizeof(std::string_view::value_type) == sizeof(std::u8string_view::value_type)
                  && sizeof(std::string::value_type) == sizeof(std::u8string::value_type),
              "btu::common string assumption violated");

constexpr auto str_compare(std::u8string_view lhs,
                           std::u8string_view rhs,
                           CaseSensitive case_sensitive) -> bool
{
    if (lhs.size() != rhs.size())
        return false;

    detail::assert_valid_utf8(lhs);
    detail::assert_valid_utf8(rhs);

    const auto f = case_sensitive == CaseSensitive::Yes ? utf8ncmp : utf8ncasecmp;
    return f(lhs.data(), rhs.data(), lhs.size()) == 0;
}

constexpr auto str_find(std::u8string_view string,
                        std::u8string_view snippet,
                        CaseSensitive case_sensitive) -> size_t
{
    detail::assert_valid_utf8(string);
    detail::assert_valid_utf8(snippet);

    using std::cbegin, std::cend;

    const auto f    = case_sensitive == CaseSensitive::Yes ? utf8str : utf8casestr;
    const auto *ptr = f(string.data(), snippet.data());

    if (ptr == nullptr)
        return std::u8string::npos;

    return ptr - string.data();
}

constexpr auto str_contain(std::u8string_view string,
                           std::u8string_view snippet,
                           CaseSensitive case_sensitive) -> bool
{
    return str_find(string, snippet, case_sensitive) != std::string::npos;
}

constexpr auto str_starts_with(std::u8string_view string,
                               std::u8string_view snippet,
                               CaseSensitive case_sensitive) -> bool
{
    const auto string_with_prefix_len = string.substr(0, snippet.size());
    return str_compare(string_with_prefix_len, snippet, case_sensitive);
}

constexpr auto str_trim(std::u8string_view in) noexcept -> std::u8string_view
{
    if (in.empty())
        return {};

    auto pred = [](char8_t c) { return isspace(c) || c == u8'\0'; };

    const auto begin = std::ranges::find_if_not(in, pred);
    const auto end   = std::ranges::find_if_not(in | std::views::reverse, pred).base();

    if (begin >= end)
        return std::u8string_view{};

    return std::u8string_view(&*begin, end - begin);
}

constexpr auto first_codepoint(std::u8string_view string) -> U8Unit
{
    U8Unit res{};
    utf8codepoint(string.data(), &res);
    return res;
}

constexpr auto is_lower(std::u8string_view string) -> bool
{
    return std::all_of(UTF8Iterator(string), UTF8Iterator::end(string), utf8islower);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): some things are just complex
constexpr auto str_match(std::u8string_view string,
                         std::u8string_view pattern,
                         CaseSensitive case_sensitive,
                         Cards cards) -> bool
{
    // Empty pattern can only match with empty sting
    if (pattern.empty())
        return string.empty();

    auto pat_it        = UTF8Iterator(pattern);
    const auto pat_end = UTF8Iterator::end(pattern);

    auto str_it        = UTF8Iterator(string);
    const auto str_end = UTF8Iterator::end(string);

    auto anyrep_pos_pat = pat_end;
    auto anyrep_pos_str = str_end;

    auto set_pos_pat = pat_end;

    while (str_it != str_end)
    {
        U8Unit current_pat = 0;
        U8Unit current_str = -1;
        if (pat_it != pat_end)
        {
            current_pat = case_sensitive == CaseSensitive::Yes ? *pat_it : utf8lwrcodepoint(*pat_it);
            current_str = case_sensitive == CaseSensitive::Yes ? *str_it : utf8lwrcodepoint(*str_it);
        }
        if (pat_it != pat_end && current_pat == cards.set_begin)
        {
            set_pos_pat = pat_it;
            ++pat_it;
        }
        else if (pat_it != pat_end && current_pat == cards.set_end)
        {
            if (anyrep_pos_pat != pat_end)
            {
                set_pos_pat = pat_end;
                ++pat_it;
            }
            else
            {
                return false;
            }
        }
        else if (set_pos_pat != pat_end)
        {
            if (current_pat == current_str)
            {
                set_pos_pat = pat_end;
                pat_it      = std::next(std::find(pat_it, pat_end, cards.set_end));
                ++str_it;
            }
            else
            {
                if (pat_it == pat_end)
                {
                    return false;
                }
                ++pat_it;
            }
        }
        else if (pat_it != pat_end && (current_pat == current_str || current_pat == cards.any))
        {
            ++pat_it;
            ++str_it;
        }
        else if (pat_it != pat_end && current_pat == cards.any_repeat)
        {
            anyrep_pos_pat = pat_it;
            anyrep_pos_str = str_it;
            ++pat_it;
        }
        else if (anyrep_pos_pat != pat_end)
        {
            pat_it = std::next(anyrep_pos_pat);
            str_it = std::next(anyrep_pos_str);
            ++anyrep_pos_str;
        }
        else
        {
            return false;
        }
    }
    while (pat_it != pat_end)
    {
        const U8Unit cur = case_sensitive == CaseSensitive::Yes ? *pat_it : utf8lwrcodepoint(*pat_it);
        if (cur == cards.any_repeat)
            ++pat_it;
        else
            break;
    }
    return pat_it == pat_end;
}

} // namespace btu::common
