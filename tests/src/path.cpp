/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/path.hpp"

#include <catch.hpp>

TEST_CASE("to_lower path", "[path]")
{
    namespace c = btu::common;
    auto p      = c::Path("somedir");
    REQUIRE(c::to_lower(p) == p);
}
