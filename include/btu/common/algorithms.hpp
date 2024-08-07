/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <type_traits>
#include <vector>

namespace btu::common {
//Expects a range sorted in descending order
template<class It, class Predicate, class Sum, class ValueType = typename std::iterator_traits<It>::value_type>
    requires std::bidirectional_iterator<It> && std::invocable<Predicate, ValueType>
                 && std::invocable<Sum, ValueType, ValueType>
[[nodiscard]] auto merge_if(It first, It last, const Predicate &predicate, const Sum &sum) -> It
{
    if (first == last)
        return last;

    --last;
    while (first != last)
    {
        while (predicate(*first, *last))
        {
            *first = sum(*first, *last);
            --last;
            if (first == last)
                return ++first;
        }
        ++first;
    }
    return ++first;
}

template<class It, class Predicate>
[[nodiscard]] auto merge_if(It first, It last, Predicate &&predicate) -> It
{
    using Type = typename std::iterator_traits<It>::value_type;
    auto plus  = [](const Type &lhs, const Type &rhs) { return lhs + rhs; };
    return merge_if(first, last, std::forward<Predicate>(predicate), plus);
}

template<class Container, class Predicate>
[[nodiscard]] auto merge_if(Container &cont, Predicate &&predicate)
{
    using std::begin, std::end;
    return merge_if(begin(cont), end(cont), std::forward<Predicate>(predicate));
}

template<class Container, class Predicate, class Sum>
[[nodiscard]] auto merge_if(Container &cont, Predicate &&pred, Sum &&sum)
{
    using std::begin, std::end;
    return merge_if(begin(cont), end(cont), forward<Predicate>(pred), forward<Sum>(sum));
}

template<class Container, class ValueType>
[[nodiscard]] auto contains(const Container &cont, const ValueType &val)
{
    return std::ranges::find(cont, val) != end(cont);
}

template<class ValueType>
void remove_duplicates(std::vector<ValueType> &cont)
{
    std::sort(begin(cont), end(cont));
    cont.erase(std::unique(begin(cont), end(cont)), end(cont));
}

} // namespace btu::common
