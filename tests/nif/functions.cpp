/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "utils.hpp"

TEST_CASE("borked nif", "[src]")
{
    const Path dir = "nif_borked";

    REQUIRE_FALSE(btu::nif::load(dir / "input.nif").has_value());
}

TEST_CASE("nif convert", "[src]")
{
    const Path dir = "nif_convert";

    auto tester = [](btu::nif::Mesh m) {
        return btu::nif::convert(std::move(m), btu::nif::HeadpartStatus::No, btu::Game::SSE);
    };

    test_expected(dir, "crashing.nif", tester);
    test_expected(dir, u8"i18nほん.nif", tester);
}
