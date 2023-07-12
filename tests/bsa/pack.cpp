/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"

#include <btu/bsa/unpack.hpp>

#include <iostream>

TEST_CASE("Pack", "[src]")
{
    const Path dir = "pack";
    btu::fs::remove_all(dir / "output");
    auto test_pack = [&dir](auto game, auto name) {
        using namespace btu::bsa;

        const auto sets = Settings::get(game);

        auto errs = pack(PackSettings{
            .input_dir     = dir / "input",
            .output_dir    = dir / "output",
            .game_settings = sets,
            .compress      = Compression::Yes,
            .archive_name_gen =
                [name, &sets](ArchiveType type) {
                    return std::u8string(name)
                           + (type == ArchiveType::Textures ? u8" - Textures" : u8" - Main") + sets.extension;
                },
        });

        if (!errs.empty())
        {
            std::cerr << "Errors while packing " << btu::common::as_ascii(name) << ":\n";
            for (const auto &[file, exc] : errs)
            {
                try
                {
                    std::rethrow_exception(exc);
                }
                catch (const std::exception &e)
                {
                    std::cerr << file.string() << ": " << e.what() << '\n';
                }
            }
            FAIL();
        }
    };

    test_pack(btu::Game::SSE, u8"sse");
    test_pack(btu::Game::FO4, u8"fo4");

    CHECK(btu::common::compare_directories(dir / "output", dir / "expected"));
}
