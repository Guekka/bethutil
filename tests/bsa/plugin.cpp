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
    SECTION("base")
    {
        auto dir = prepare_dir(
            std::vector{u8"dummy_sse.esp"sv, u8"dummy_sse - Textures.bsa"sv, u8"another.bsa"sv});

        create_file(dir.path() / u8"dummy_sse.esp", Settings::get(btu::Game::SSE).dummy_plugin.value());
        remake_dummy_plugins(dir.path(), Settings::get(btu::Game::SSE));

        CHECK(btu::fs::exists(dir.path() / u8"dummy_sse.esp"));
        CHECK(btu::fs::exists(dir.path() / u8"another.esp"));

        auto dummy_content = require_expected(btu::common::read_file(dir.path() / u8"dummy_sse.esp"));

        CHECK(dummy_content == Settings::get(btu::Game::SSE).dummy_plugin.value());
    }
}
