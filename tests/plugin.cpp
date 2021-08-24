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
        CHECK(plug->dir_ == "C:/SomeDir");
        CHECK(plug->name_ == Path("Requiem"));
        CHECK(plug->suffix_ == Path{});
        CHECK(plug->ext_ == ".esp");
    }
    SECTION("Complex (SSE)")
    {
        auto sets = Settings::get(Game::SSE);

        auto plug  = FilePath::make("C:/SomeDir/Requiem - Textures01.bsa", sets, FileTypes::BSA);
        auto plug2 = FilePath::make("C:/SomeDir/Requiem01 - Textures.bsa", sets, FileTypes::BSA);
        REQUIRE(plug.has_value());
        CHECK(plug->counter_ == plug2->counter_); // Digits can be after or before suffix
        CHECK(plug->dir_ == "C:/SomeDir");
        CHECK(plug->name_ == BETHUTIL_BSA_STR("Requiem"));
        CHECK(plug->suffix_ == BETHUTIL_BSA_STR("Textures"));
        CHECK(plug->counter_.value() == 1);
        CHECK(plug->ext_ == ".bsa");
    }
    SECTION("Complex 2 (SSE)")
    {
        auto sets = Settings::get(Game::SSE);
        auto plug = FilePath::make("C:/AnotherSomeDir/Requiem01 - Enhancement - Textures.bsa",
                                   sets,
                                   FileTypes::BSA);

        REQUIRE(plug.has_value());
        CHECK(plug->dir_ == "C:/AnotherSomeDir");
        CHECK(plug->name_ == BETHUTIL_BSA_STR("Requiem01 - Enhancement"));
        CHECK(plug->suffix_ == BETHUTIL_BSA_STR("Textures"));
        CHECK(!plug->counter_.has_value());
        CHECK(plug->ext_ == ".bsa");
    }
    SECTION("Complex 3 (SSE)")
    {
        auto sets = Settings::get(Game::SSE);
        auto plug = FilePath::make("C:/AnotherSomeDir/Requiem - Enhancement01.bsa", sets, FileTypes::BSA);

        REQUIRE(plug.has_value());
        CHECK(plug->dir_ == "C:/AnotherSomeDir");
        CHECK(plug->name_ == BETHUTIL_BSA_STR("Requiem - Enhancement"));
        CHECK(plug->suffix_ == BETHUTIL_BSA_STR(""));
        CHECK(plug->counter_.value() == 1);
        CHECK(plug->ext_ == ".bsa");
    }
}
