/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/metaprogramming.hpp"

#include <catch2/catch.hpp>

TEST_CASE("is_equiv", "[metaprogramming]")
{
    using btu::common::is_equiv_v;
    STATIC_REQUIRE(is_equiv_v<const int &, int>);
    STATIC_REQUIRE(is_equiv_v<const int &&, int>);
    STATIC_REQUIRE(is_equiv_v<std::vector<int> &, std::vector<int>>);

    STATIC_REQUIRE_FALSE(is_equiv_v<std::vector<int &>, std::vector<int>>);
}

TEST_CASE("to_underlying", "[metaprogramming]")
{
    using btu::common::to_underlying;
    enum class E : std::uint16_t
    {
        val = 4
    };

    STATIC_REQUIRE(std::is_same_v<decltype(to_underlying(E::val)), std::uint16_t>);
    STATIC_REQUIRE(to_underlying(E::val) == 4);
}

TEST_CASE("overload", "[metaprogramming]")
{
    using btu::common::overload;
    auto callable = overload{
        [](int) { return int{}; },
        [](float) { return float{}; },
        [](double) { return double{}; },
    };

    auto check = []<typename T>(T) { STATIC_REQUIRE(std::is_same_v<decltype(callable(T{})), T>); };
    check(int{});
    check(float{});
    check(double{});
}
