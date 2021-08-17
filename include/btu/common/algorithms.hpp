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
template<class It, class Predicate, class Sum>
[[nodiscard]] inline It merge_if(It first, It last, const Predicate &predicate, const Sum &sum)
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
[[nodiscard]] inline It merge_if(It first, It last, Predicate &&predicate)
{
    using Type = typename std::iterator_traits<It>::value_type;
    auto plus  = [](const Type &first, const Type &second) { return first + second; };
    return merge_if(first, last, std::forward<Predicate>(predicate), plus);
}

template<class Container, class Predicate>
[[nodiscard]] inline auto merge_if(Container &cont, Predicate &&predicate)
{
    using namespace std;
    return merge_if(begin(cont), end(cont), forward<Predicate>(predicate));
}

template<class Container, class Predicate, class Sum>
[[nodiscard]] inline auto merge_if(Container &cont, Predicate &&pred, Sum &&sum)
{
    using namespace std;
    return merge_if(begin(cont), end(cont), forward<Predicate>(pred), forward<Sum>(sum));
}

template<class Container, class ValueType>
[[nodiscard]] inline auto contains(const Container &cont, const ValueType &val)
{
    using namespace std;
    return find(begin(cont), end(cont), val) != end(cont);
}

template<class Container, class Predicate>
[[nodiscard]] inline auto find_if(const Container &cont, const Predicate &pred)
{
    using namespace std;
    return find_if(begin(cont), end(cont), pred);
}

template<typename Cont, typename Pred>
void erase_if(Cont &cont, const Pred &pred)
{
    using namespace std;
    cont.erase(remove_if(begin(cont), end(cont), pred), end(cont));
}
} // namespace btu::common
