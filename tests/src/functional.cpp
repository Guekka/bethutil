/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/functional.hpp"

#include <catch.hpp>

TEST_CASE("bind_back")
{
    using btu::common::bind_back;

    auto func = [](int a, std::unique_ptr<int> &b, std::unique_ptr<int> c) { return a + *b + *c; };

    auto init      = [] { return std::make_tuple(1, std::make_unique<int>(2), std::make_unique<int>(3)); };
    auto [a, b, c] = init();
    CHECK(func(a, b, std::move(c)) == 6);
    std::tie(a, b, c) = init();
    CHECK(bind_back(func, std::move(c))(a, b) == 6);
    std::tie(a, b, c) = init();
    CHECK(bind_back(func, std::ref(b), std::move(c))(a) == 6);
}
