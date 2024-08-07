/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/filesystem.hpp"
#include "utils.hpp"

using namespace btu::bsa;

[[nodiscard]] auto prepare_dir(std::span<const std::u8string_view> filenames) -> TempPath
{
    auto dir = TempPath(btu::fs::temp_directory_path() / "bsa_plugin");
    btu::fs::create_directories(dir.path());

    for (const auto &filename : filenames)
        create_file(dir.path() / filename);

    return dir;
}

TEST_CASE("clean_dummy_plugins", "[src]")
{
    const auto &sets = Settings::get(btu::Game::SSE);

    SECTION("base case")
    {
        auto dir = prepare_dir({});
        create_file(dir.path() / u8"dummy_sse.esp", sets.dummy_plugin.value());
        const auto plugins = list_plugins(dir.path(), sets);
        CHECK(plugins == std::vector{dir.path() / u8"dummy_sse.esp"});
        clean_dummy_plugins(plugins, sets);
        CHECK(btu::fs::is_empty(dir.path()));
    }

    SECTION("empty directory")
    {
        const auto dir     = prepare_dir({});
        const auto plugins = list_plugins(dir.path(), sets);
        CHECK(plugins.empty());
        clean_dummy_plugins(plugins, sets);
        CHECK(is_empty(dir.path()));
    }
}

TEST_CASE("find_archive_name", "[src]")
{
    auto sets = Settings::get(btu::Game::SSE);

    SECTION("empty directory uses directory name")
    {
        const auto dir      = prepare_dir({});
        auto result         = find_archive_name(dir.path(), sets, ArchiveType::Standard);
        const auto expected = dir.path() / dir.path().filename() += sets.extension;
        CHECK(result.value_or("") == expected);
    }

    SECTION("finds a name if a plugin exists")
    {
        const auto dir = prepare_dir(std::vector{u8"dummy_sse.esp"sv});

        auto result = find_archive_name(dir.path(), sets, ArchiveType::Textures);
        CHECK(result.has_value());
        CHECK(result->filename() == u8"dummy_sse - Textures.bsa");
    }

    SECTION("finds a name with counter if archives already exist")
    {
        const auto dir = prepare_dir(std::vector{u8"a.esp"sv, u8"a.bsa"sv, u8"a - Textures.bsa"sv});

        auto result = find_archive_name(dir.path(), sets, ArchiveType::Textures);
        CHECK(result.has_value());
        INFO(result.value().string());
        CHECK(result->filename() == Path("a1 - Textures.bsa"));
    }
}

TEST_CASE("remake dummy plugins")
{
    GIVEN("a game that can load limited archives per plugin, a directory with no plugins, and two archives")
    {
        auto dir = prepare_dir(std::vector{u8"Test.bsa"sv, u8"Test1.bsa"sv});

        WHEN("remake_dummy_plugins is called")
        {
            auto sets = Settings::get(btu::Game::SSE);
            remake_dummy_plugins(dir.path(), sets);

            THEN("two dummy plugins were created")
            {
                CHECK(list_plugins(dir.path(), sets).size() == 2);
            }
            AND_THEN("the dummy plugins have the correct content")
            {
                auto dummy_content = require_expected(btu::common::read_file(dir.path() / u8"Test.esp"));
                CHECK(dummy_content == sets.dummy_plugin.value());
                dummy_content = require_expected(btu::common::read_file(dir.path() / u8"Test1.esp"));
                CHECK(dummy_content == sets.dummy_plugin.value());
            }
        }
    }
    GIVEN("a game that can load unlimited archives per plugin, a directory with no plugins, and two archives")
    {
        auto dir = prepare_dir(std::vector{u8"Test.bsa"sv, u8"Test1.bsa"sv});

        WHEN("remake_dummy_plugins is called")
        {
            auto sets = Settings::get(btu::Game::FNV);
            remake_dummy_plugins(dir.path(), sets);

            THEN("one dummy plugin was created")
            {
                CHECK(list_plugins(dir.path(), sets).size() == 1);
            }
            AND_THEN("the dummy plugin has the correct content")
            {
                auto dummy_content = require_expected(btu::common::read_file(dir.path() / u8"Test.esp"));
                CHECK(dummy_content == sets.dummy_plugin.value());
            }
        }
    }
    GIVEN("a directory with a plugin, a loaded archive and an unloaded archive")
    {
        auto dir = prepare_dir(
            std::vector{u8"existing.esp"sv, u8"existing - Textures.bsa"sv, u8"unloaded.bsa"sv});

        WHEN("remake_dummy_plugins is called")
        {
            auto sets = Settings::get(btu::Game::SSE);
            remake_dummy_plugins(dir.path(), sets);

            THEN("the dummy plugin has the correct content")
            {
                auto dummy_content = require_expected(btu::common::read_file(dir.path() / u8"unloaded.esp"));
                CHECK(dummy_content == sets.dummy_plugin.value());
            }
            AND_THEN("the existing plugin was not modified")
            {
                auto existing_content = require_expected(
                    btu::common::read_file(dir.path() / u8"existing.esp"));
                CHECK(existing_content.empty());
            }
        }
    }
    GIVEN("a directory with a plugin that can load a limited number of archives, a loaded archive \
           and an unloaded archive with the same prefix")
    {
        auto dir = prepare_dir(std::vector{u8"plugin.esp"sv, u8"plugin.bsa"sv, u8"plugin1.bsa"sv});

        WHEN("remake_dummy_plugins is called")
        {
            auto sets = Settings::get(btu::Game::SSE);
            remake_dummy_plugins(dir.path(), sets);

            THEN("the dummy plugin has the correct content")
            {
                auto dummy_content = require_expected(btu::common::read_file(dir.path() / u8"plugin1.esp"));
                CHECK(dummy_content == sets.dummy_plugin.value());
            }
            AND_THEN("the existing plugin was not modified")
            {
                auto existing_content = require_expected(btu::common::read_file(dir.path() / u8"plugin.esp"));
                CHECK(existing_content.empty());
            }
        }
    }
    GIVEN("a directory with a plugin that can load an unlimited number of archives and two loaded archives")
    {
        auto dir = prepare_dir(std::vector{u8"plugin.esp"sv, u8"plugin.bsa"sv, u8"plugin1.bsa"sv});

        WHEN("remake_dummy_plugins is called")
        {
            auto sets = Settings::get(btu::Game::FNV);
            remake_dummy_plugins(dir.path(), sets);

            THEN("the existing plugin is the single plugin in the folder")
            {
                const auto plugins = list_plugins(dir.path(), sets);
                CHECK(plugins == std::vector{dir.path() / u8"plugin.esp"});
            }
            AND_THEN("the existing plugin was not modified")
            {
                auto existing_content = require_expected(btu::common::read_file(dir.path() / u8"plugin.esp"));
                CHECK(existing_content.empty());
            }
        }
    }
}
