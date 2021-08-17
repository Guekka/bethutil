/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

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

template<class CharT>
bool str_compare(std::basic_string_view<CharT> string,
                 std::basic_string_view<CharT> other,
                 bool case_sensitive = true)
{
    auto pred = str_compare_pred<CharT>(case_sensitive);

    using namespace std;
    return string.size() == other.size() && equal(cbegin(other), cend(other), cbegin(string), pred);
}

template<class CharT>
std::basic_string<CharT> to_lower(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> res;
    res.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(res), [](auto &&c) { return ::tolower(c); });
    return res;
}
} // namespace btu::common
