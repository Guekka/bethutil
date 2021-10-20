/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/string.hpp"

#include <catch2/catch.hpp>

TEST_CASE("to_lower", "[string]")
{
    using btu::common::to_lower;

    using namespace std::literals;

    SECTION("Basic ASCII")
    {
        CHECK(to_lower(u8"A"sv) == u8"a");
        CHECK(to_lower(u8"ABCDEF"sv) == u8"abcdef");
        CHECK(to_lower(u8"abcdef"sv) == u8"abcdef");
        CHECK(to_lower(u8"!"sv) == u8"!");
    }

    SECTION("More complicated characters are supported") { CHECK(to_lower(u8"À"sv) == u8"à"); }
}

TEST_CASE("str_compare", "[string]")
{
    using btu::common::str_compare;

    SECTION("Basic ASCII")
    {
        CHECK_FALSE(str_compare(u8"A", u8"a"));
        CHECK(str_compare(u8"A", u8"a", false));

        CHECK_FALSE(str_compare(u8"somepath/c/x/d!", u8"somepath/C/X/D!"));
        CHECK(str_compare(u8"somepath/c/x/d!", u8"somepath/C/X/D!", false));
    }
}
