/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "utils.hpp"

TEST_CASE("Plugin names are correctly parsed")
{
    SECTION("Simple (SSE)")
    {
        auto sets = Settings::get(Game::SSE);

        REQUIRE_FALSE(FilePath::make("", sets, FileTypes::Plugin).has_value());
        auto plug = FilePath::make("C:/SomeDir/Requiem.esp", sets, FileTypes::Plugin);
        REQUIRE(plug.has_value());
        CHECK(plug->dir == "C:/SomeDir");
        CHECK(plug->name == u8"Requiem");
        CHECK(plug->suffix == u8"");
        CHECK(plug->ext == u8".esp");
    }
    SECTION("Complex (SSE)")
    {
        auto sets = Settings::get(Game::SSE);

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
        auto sets = Settings::get(Game::SSE);
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
        auto sets = Settings::get(Game::SSE);
        auto plug = FilePath::make("C:/AnotherSomeDir/Requiem - Enhancement01.bsa", sets, FileTypes::BSA);

        REQUIRE(plug.has_value());
        CHECK(plug->dir == "C:/AnotherSomeDir");
        CHECK(plug->name == u8"Requiem - Enhancement");
        CHECK(plug->suffix == u8"");
        CHECK(plug->counter.value() == 1);
        CHECK(plug->ext == u8".bsa");
    }
}
