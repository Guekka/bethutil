/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "../utils.hpp"
#include "btu/bsa/plugin.hpp"
#include "btu/common/filesystem.hpp"

#include <iostream>

TEST_CASE("Pack", "[src]")
{
    auto pack = [](auto game, auto name) {
        using namespace btu::bsa;
        const Path dir  = "pack";
        const auto sets = Settings::get(game);
        auto archs      = split(dir, sets);
        REQUIRE(archs.size() == 3);
        merge(archs);
        REQUIRE(archs.size() == 1);

        const std::array plugins = {FilePath(dir, u8"plug", u8"", u8"esp", FileTypes::Plugin)};
        auto arch                = archs.back();
        const auto out           = find_archive_name(plugins, sets, arch.get_type()).full_path();
        arch.set_out_path(out);
        auto errs = write(true, std::move(arch), dir);
        REQUIRE(errs.empty());

        REQUIRE(btu::common::compare_files(out, dir / (u8"expected_"s + name + sets.extension)));
        fs::remove(out);
    };

    pack(btu::common::Game::SSE, u8"sse");
#ifdef _MSC_VER // FO4dds does not work on Linux
    pack(btu::common::Game::FO4, u8"fo4");
#endif
}
