/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/metaprogramming.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>

namespace btu::common {

constexpr auto to_string_view = overload{
    [](const char *s) { return std::string_view(s); },
    [](const std::string &s) { return std::string_view(s); },
    [](const wchar_t *s) { return std::wstring_view(s); },
    [](const std::wstring &s) { return std::wstring_view(s); },
    [](const auto &s) { return std::basic_string_view(s); },
};

template<typename CharT>
using StrComparePred = bool (*)(CharT, CharT);

template<typename CharT>
StrComparePred<CharT> str_compare_pred(bool case_sensitive = true)
{
    if (case_sensitive)
        return [](CharT ch1, CharT ch2) { return ch1 == ch2; };

    return [](CharT ch1, CharT ch2) {
        ch1 = ::tolower(ch1);
        ch2 = ::tolower(ch2);

        return ch1 == ch2;
    };
}

template<class CharT>
bool str_compare(std::basic_string_view<CharT> lhs,
                 std::basic_string_view<CharT> rhs,
                 bool case_sensitive = true)
{
    auto pred = str_compare_pred<CharT>(case_sensitive);

    using namespace std;
    return lhs.size() == rhs.size() && equal(cbegin(rhs), cend(rhs), cbegin(lhs), pred);
}

template<class String1, typename String2>
bool str_compare(String1 lhs, String2 rhs, bool case_sensitive = true)
{
    static_assert(is_equiv_v<decltype(lhs), decltype(rhs)>);
    return str_compare(to_string_view(lhs), to_string_view(rhs), case_sensitive);
}

template<class CharT>
std::basic_string<CharT> to_lower(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> res;
    res.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(res), [](auto &&c) { return ::tolower(c); });
    return res;
}

template<class String>
auto to_lower(String str)
{
    return to_lower(to_string_view(str));
}
} // namespace btu::common
