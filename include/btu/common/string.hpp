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

template<class String1, typename String2>
bool str_compare(String1 string, String2 other, bool case_sensitive = true)
{
    const auto lhs = std::basic_string_view(string);
    const auto rhs = std::basic_string_view(other);

    static_assert(is_equiv_v<decltype(lhs), decltype(rhs)>);
    using CharT = typename decltype(lhs)::value_type;

    auto pred = str_compare_pred<CharT>(case_sensitive);

    using namespace std;
    return lhs.size() == rhs.size() && equal(cbegin(rhs), cend(rhs), cbegin(lhs), pred);
}

template<class StringView>
auto to_lower(StringView str) ->
    typename std::basic_string<typename decltype(std::basic_string_view(str))::value_type>
{
    const auto view = std::basic_string_view(str);
    using CharT     = typename decltype(view)::value_type;
    std::basic_string<CharT> res;
    res.reserve(view.size());
    std::transform(view.begin(), view.end(), std::back_inserter(res), [](auto &&c) { return ::tolower(c); });
    return res;
}
} // namespace btu::common
