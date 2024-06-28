/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/esp/error_code.hpp"
#include "utils.hpp"

#include <btu/bsa/unpack.hpp>
#include <btu/common/filesystem.hpp>

auto unpack_all(const Path &in, const Path &out, const btu::bsa::Settings &sets)
{
    auto archives = list_archive(std::filesystem::directory_iterator(in),
                                 std::filesystem::directory_iterator(),
                                 sets);

    for (auto &&arch : archives)
    {
        const auto res = btu::bsa::unpack({.file_path = arch.full_path(), .root_opt = &out});
        CHECK(res == btu::bsa::UnpackResult::Success);
    }
}

TEST_CASE("unpack", "[src]")
{
    const Path dir = "bsa_unpack";
    const Path in  = dir / "in";
    btu::fs::remove_all(dir / "out");

    unpack_all(in, dir / "out", btu::bsa::Settings::get(btu::Game::SSE));
    unpack_all(in, dir / "out", btu::bsa::Settings::get(btu::Game::FO4));

    REQUIRE(btu::common::compare_directories(dir / "out", dir / "expected"));
}
