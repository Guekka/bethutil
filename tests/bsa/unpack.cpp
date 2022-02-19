/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/unpack.hpp"

#include "btu/common/filesystem.hpp"
#include "utils.hpp"

#ifdef _MSC_VER // Textures BA2
TEST_CASE("unpack", "[src]")
{
    const Path dir = "bsa_unpack";
    const Path in  = dir / "in";
    fs::remove_all(dir / "out");

    btu::bsa::unpack_all(in, dir / "out", Settings::get(btu::Game::SSE));
    btu::bsa::unpack_all(in, dir / "out", Settings::get(btu::Game::FO4));

    REQUIRE(btu::common::compare_directories(dir / "out", dir / "expected"));
}
#endif
