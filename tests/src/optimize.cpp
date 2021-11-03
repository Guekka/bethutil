/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/optimize.hpp"

#include <DirectXTex.h>
#include <catch.hpp>

TEST_CASE("optimize")
{
    CHECK(btu::tex::optimize({}, {}).has_value());
}
