/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/optimize.hpp"

#include "btu/tex/texture.hpp"

#include <catch.hpp>

TEST_CASE("optimize")
{
    SECTION("compiles")
    {
        auto tex         = btu::tex::Texture{};
        const auto &sets = btu::tex::Settings::get(btu::common::Game::SSE);
        const auto steps = btu::tex::compute_optimization_steps(tex, sets);
        const auto res   = btu::tex::optimize(std::move(tex), steps);
    }
    CHECK_FALSE(btu::tex::optimize({}, {}).has_value());
}
