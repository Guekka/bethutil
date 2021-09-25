/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/string.hpp"

#include <catch2/catch.hpp>

TEST_CASE("to_lower", "[string]")
{
    using btu::common::to_lower;
    auto check       = [](std::string_view upper, auto lower) { CHECK(to_lower(upper) == lower); };
    auto check_false = [](std::string_view upper, auto lower) { CHECK_FALSE(to_lower(upper) == lower); };

    SECTION("Basic ASCII")
    {
        check("A", "a");
        check("ABCDEF", "abcdef");
        check("abcdef", "abcdef");
        check("!", "!");
    }
    SECTION("More complicated characters are NOT supported") { check_false("À", "à"); }
}

TEST_CASE("str_compare", "[string]")
{
    using btu::common::str_compare;

    SECTION("Basic ASCII")
    {
        CHECK_FALSE(str_compare("A", "a"));
        CHECK(str_compare("A", "a", false));

        CHECK_FALSE(str_compare("somepath/c/x/d!", "somepath/C/X/D!"));
        CHECK(str_compare("somepath/c/x/d!", "somepath/C/X/D!", false));
    }
}
