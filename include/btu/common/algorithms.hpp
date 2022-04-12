/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>

namespace btu::common {
//Expects a range sorted in descending order
template<class It, class Predicate, class Sum, class ValueType = typename std::iterator_traits<It>::value_type>
requires std::bidirectional_iterator<
    It> && std::invocable<Predicate, ValueType> && std::invocable<Sum, ValueType, ValueType>
[[nodiscard]] inline auto merge_if(It first, It last, const Predicate &predicate, const Sum &sum) -> It
{
    if (first == last)
        return last;

    last--;
    while (first != last)
    {
        while (predicate(*first, *last))
        {
            *first = sum(*first, *last);
            last--;
            if (first == last)
                return ++first;
        }
        first++;
    }
    return ++first;
}

template<class It, class Predicate>
[[nodiscard]] inline auto merge_if(It first, It last, Predicate &&predicate) -> It
{
    using Type = typename std::iterator_traits<It>::value_type;
    auto plus  = [](const Type &first, const Type &second) { return first + second; };
    return merge_if(first, last, std::forward<Predicate>(predicate), plus);
}

template<class Container, class Predicate>
[[nodiscard]] inline auto merge_if(Container &cont, Predicate &&predicate)
{
    using std::begin, std::end;
    return merge_if(begin(cont), end(cont), std::forward<Predicate>(predicate));
}

template<class Container, class Predicate, class Sum>
[[nodiscard]] inline auto merge_if(Container &cont, Predicate &&pred, Sum &&sum)
{
    using std::begin, std::end;
    return merge_if(begin(cont), end(cont), forward<Predicate>(pred), forward<Sum>(sum));
}

template<class Container, class ValueType>
[[nodiscard]] inline auto contains(const Container &cont, const ValueType &val)
{
    return std::ranges::find(cont, val) != end(cont);
}
} // namespace btu::common
