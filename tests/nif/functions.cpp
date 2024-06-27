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
        return convert(std::move(m), btu::nif::HeadpartStatus::No, btu::Game::SSE);
    };

    test_expected(dir, "crashing.nif", tester);
    test_expected(dir, u8"i18nほん.nif", tester);
}

TEST_CASE("Nif Memory IO", "[src]")
{
    const Path dir  = "nif_memory_io";
    const Path file = dir / "in" / "crashing.nif";

    // load
    auto data    = btu::common::read_file(file);
    auto mem_nif = btu::nif::load(file, data);
    auto fs_nif  = btu::nif::load(file);

    REQUIRE(mem_nif.has_value());
    REQUIRE(fs_nif.has_value());

    // save
    auto mem_data = save(mem_nif.value());
    REQUIRE(mem_data.has_value());

    auto out = dir / "out" / "crashing.nif";
    btu::fs::create_directories(out.parent_path());

    REQUIRE(btu::nif::save(mem_nif.value(), out));

    auto fs_data = btu::common::read_file(out);

    REQUIRE(*mem_data == fs_data);
}
