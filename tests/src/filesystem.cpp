/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/filesystem.hpp"

#include "catch2/catch.hpp"

TEST_CASE("read_file", "[fs]")
{
    SECTION("invalid path throw exception") { REQUIRE_THROWS(btu::common::read_file("")); }
    SECTION("100space.bin")
    {
        constexpr auto file = "read_file/100space.bin";
        const auto content  = std::string(100, ' ');
        const auto data     = btu::common::read_file(file);
        REQUIRE(data.size() == btu::common::fs::file_size(file));
        REQUIRE(std::equal(data.cbegin(), data.cend(), content.cbegin(), [](auto byte, auto c) {
            return static_cast<std::byte>(c) == byte;
        }));
    }
}
