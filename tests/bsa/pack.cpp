/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"

#include <iostream>

TEST_CASE("Pack", "[src]")
{
    const Path dir = "pack";
    btu::fs::remove_all(dir / "output");
    auto pack = [&dir](auto game, auto name) {
        using namespace btu::bsa;

        const auto sets = Settings::get(game);
        auto archs      = split(dir / "input", sets);
        REQUIRE(archs.size() == 1);
        merge(archs);
        REQUIRE(archs.size() == 1);

        auto out  = dir / "output" / (name + sets.extension);
        auto errs = write(out, Compression::Yes, std::move(archs.back()));

        if (!errs.empty())
        {
            std::cerr << "Errors while packing " << btu::common::as_ascii(name) << ":\n";
            for (const auto &[file, err] : errs)
                std::cerr << file << ": " << err << '\n';
            FAIL();
        }
    };

    pack(btu::Game::SSE, u8"sse");
    pack(btu::Game::FO4, u8"fo4");

    CHECK(btu::common::compare_directories(dir / "output", dir / "expected"));
}
