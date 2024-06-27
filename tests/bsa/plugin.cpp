/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/filesystem.hpp"
#include "utils.hpp"

using namespace btu::bsa;

TEST_CASE("Plugin names are correctly parsed", "[src]")
{
    SECTION("Simple (SSE)")
    {
        auto sets = Settings::get(btu::Game::SSE);

        REQUIRE_FALSE(FilePath::make("", sets, FileTypes::Plugin).has_value());
        auto plug = FilePath::make("C:/SomeDir/Requiem.esp", sets, FileTypes::Plugin);
        REQUIRE(plug.has_value());
        CHECK(plug->dir == "C:/SomeDir");
        CHECK(plug->name == u8"Requiem");
        CHECK(plug->suffix.empty());
        CHECK(plug->ext == u8".esp");
    }
    SECTION("Complex (SSE)")
    {
        auto sets = Settings::get(btu::Game::SSE);

        auto plug  = FilePath::make("C:/SomeDir/Requiem - Textures01.bsa", sets, FileTypes::BSA);
        auto plug2 = FilePath::make("C:/SomeDir/Requiem01 - Textures.bsa", sets, FileTypes::BSA);
        REQUIRE(plug.has_value());
        CHECK(plug->counter == plug2->counter); // Digits can be after or before suffix
        CHECK(plug->dir == "C:/SomeDir");
        CHECK(plug->name == u8"Requiem");
        CHECK(plug->suffix == u8"Textures");
        CHECK(plug->counter.value() == 1);
        CHECK(plug->ext == u8".bsa");
    }
    SECTION("Complex 2 (SSE)")
    {
        auto sets = Settings::get(btu::Game::SSE);
        auto plug = FilePath::make("C:/AnotherSomeDir/Requiem01 - Enhancement - Textures.bsa",
                                   sets,
                                   FileTypes::BSA);

        REQUIRE(plug.has_value());
        CHECK(plug->dir == "C:/AnotherSomeDir");
        CHECK(plug->name == u8"Requiem01 - Enhancement");
        CHECK(plug->suffix == u8"Textures");
        CHECK(!plug->counter.has_value());
        CHECK(plug->ext == u8".bsa");
    }
    SECTION("Complex 3 (SSE)")
    {
        auto sets = Settings::get(btu::Game::SSE);
        auto plug = FilePath::make("C:/AnotherSomeDir/Requiem - Enhancement01.bsa", sets, FileTypes::BSA);

        REQUIRE(plug.has_value());
        CHECK(plug->dir == "C:/AnotherSomeDir");
        CHECK(plug->name == u8"Requiem - Enhancement");
        CHECK(plug->suffix.empty());
        CHECK(plug->counter == 1U);
        CHECK(plug->ext == u8".bsa");
    }
    SECTION("Complex 4 (FO4)")
    {
        auto sets = Settings::get(btu::Game::FO4);
        auto plug = FilePath::make("C:/Mk. II - Frag.esp", sets, FileTypes::Plugin);

        REQUIRE(plug.has_value());
        CHECK(plug->name == u8"Mk. II - Frag");
        CHECK(plug->suffix.empty());
        CHECK_FALSE(plug->counter.has_value());
        CHECK(plug->ext == u8".esp");
    }
    SECTION("Complex 5 (FO4)")
    {
        auto sets = Settings::get(btu::Game::FO4);
        auto plug = FilePath::make("some_dir/Colt 6520.esp", sets, FileTypes::Plugin);

        REQUIRE(plug.has_value());
        CHECK(plug->name == u8"Colt ");
        CHECK(plug->suffix.empty());
        CHECK(plug->counter == 6520U);
        CHECK(plug->ext == u8".esp");
    }
    SECTION("Complex 6 (FO4)")
    {
        auto sets = Settings::get(btu::Game::FO4);
        auto plug = FilePath::make("some_dir/Colt. 6520.esp", sets, FileTypes::Plugin);

        REQUIRE(plug.has_value());
        CHECK(plug->name == u8"Colt. ");
        CHECK(plug->suffix.empty());
        CHECK(plug->counter == 6520U);
        CHECK(plug->ext == u8".esp");
    }
}

TEST_CASE("clean_dummy_plugins", "[src]")
{
    const Path dir = "bsa_clean_dummy_plugins";
    const Path out = dir / "out";

    btu::fs::remove_all(out);
    btu::fs::copy(dir / "in", out);

    const auto &sets = Settings::get(btu::Game::SSE);

    auto plugins = list_plugins(btu::fs::directory_iterator(out), btu::fs::directory_iterator(), sets);
    REQUIRE(plugins == std::vector{FilePath(out, u8"dummy_sse", u8"", u8".esp", FileTypes::Plugin)});
    clean_dummy_plugins(plugins, sets);
    REQUIRE(plugins.empty());
    REQUIRE(btu::common::compare_directories(out, dir / "expected"));
}
