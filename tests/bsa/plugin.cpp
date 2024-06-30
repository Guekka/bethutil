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

        CHECK_FALSE(FilePath::make("", sets, FileTypes::Plugin).has_value());
        auto plug = FilePath::make("C:/SomeDir/Requiem.esp", sets, FileTypes::Plugin);
        CHECK(plug.has_value());
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
        CHECK(plug.has_value());
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

        CHECK(plug.has_value());
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

        CHECK(plug.has_value());
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

        CHECK(plug.has_value());
        CHECK(plug->name == u8"Mk. II - Frag");
        CHECK(plug->suffix.empty());
        CHECK_FALSE(plug->counter.has_value());
        CHECK(plug->ext == u8".esp");
    }
    SECTION("Complex 5 (FO4)")
    {
        auto sets = Settings::get(btu::Game::FO4);
        auto plug = FilePath::make("some_dir/Colt 6520.esp", sets, FileTypes::Plugin);

        CHECK(plug.has_value());
        CHECK(plug->name == u8"Colt ");
        CHECK(plug->suffix.empty());
        CHECK(plug->counter == 6520U);
        CHECK(plug->ext == u8".esp");
    }
    SECTION("Complex 6 (FO4)")
    {
        auto sets = Settings::get(btu::Game::FO4);
        auto plug = FilePath::make("some_dir/Colt. 6520.esp", sets, FileTypes::Plugin);

        CHECK(plug.has_value());
        CHECK(plug->name == u8"Colt. ");
        CHECK(plug->suffix.empty());
        CHECK(plug->counter == 6520U);
        CHECK(plug->ext == u8".esp");
    }
}

TEST_CASE("FilePath::make edge cases")
{
    auto sets = Settings::get(btu::Game::SSE);

    SECTION("FilePath::make handles invalid paths", "[src]")
    {
        auto plug = FilePath::make("", sets, FileTypes::Plugin);
        CHECK_FALSE(plug.has_value());
    }

    SECTION("FilePath::make handles paths without file extension", "[src]")
    {
        auto plug = FilePath::make("C:/SomeDir/Requiem", sets, FileTypes::Plugin);
        CHECK_FALSE(plug.has_value());
    }

    SECTION("FilePath::make handles paths with unsupported file extension", "[src]")
    {
        auto plug = FilePath::make("C:/SomeDir/Requiem.txt", sets, FileTypes::Plugin);
        CHECK_FALSE(plug.has_value());
    }
}

TEST_CASE("clean_dummy_plugins", "[src]")
{
    const auto &sets = Settings::get(btu::Game::SSE);

    SECTION("base case")
    {
        const Path dir = "bsa_clean_dummy_plugins";
        const Path out = dir / "out";

        btu::fs::remove_all(out);
        btu::fs::copy(dir / "in", out);

        auto plugins = list_plugins(btu::fs::directory_iterator(out), btu::fs::directory_iterator(), sets);
        CHECK(plugins == std::vector{FilePath(out, u8"dummy_sse", u8"", u8".esp", FileTypes::Plugin)});
        clean_dummy_plugins(plugins, sets);
        CHECK(plugins.empty());
        CHECK(btu::common::compare_directories(out, dir / "expected"));
    }

    SECTION("empty directory")
    {
        const auto dir = TempPath(btu::fs::temp_directory_path());
        btu::fs::create_directories(dir.path());

        auto plugins = list_plugins(btu::fs::directory_iterator(dir.path()),
                                    btu::fs::directory_iterator(),
                                    sets);
        CHECK(plugins.empty());
        clean_dummy_plugins(plugins, sets);
        CHECK(plugins.empty());
        CHECK(is_empty(dir.path()));
    }
}

TEST_CASE("find_archive_name with plugins", "[src]")
{
    auto sets = Settings::get(btu::Game::SSE);

    SECTION("handles empty plugin list")
    {
        std::span<const FilePath> plugins;
        auto result = find_archive_name(plugins, sets, ArchiveType::Textures);
        CHECK_FALSE(result.has_value());
    }

    SECTION("returns name with incremented counter if an archive is already linked to a plugin")
    {
        const auto dir = TempPath(btu::fs::temp_directory_path());
        btu::fs::create_directories(dir.path());

        create_file(dir.path() / "ExistingPlugin.esp");
        create_file(dir.path() / "ExistingPlugin.bsa");
        create_file(dir.path() / "ExistingPlugin - Textures.bsa");

        std::vector plugins = {
            FilePath::make(dir.path() / "ExistingPlugin.esp", sets, FileTypes::Plugin).value(),
        };

        auto result = find_archive_name(plugins, sets, ArchiveType::Standard);
        REQUIRE(result.has_value());
        INFO(btu::common::as_ascii(result.value().full_path().u8string()));
        CHECK(result->name == u8"ExistingPlugin");
        CHECK(result->counter == 0);
    }
}

TEST_CASE("find_archive_name tests with directory input", "[src]")
{
    auto sets = Settings::get(btu::Game::SSE);

    SECTION("handles empty directory")
    {
        const Path dir = "empty_directory";
        auto result    = find_archive_name(dir, sets, ArchiveType::Textures);
        CHECK_FALSE(result.has_value());
    }

    SECTION("finds a name if a plugin exists")
    {
        const Path dir = "bsa_clean_dummy_plugins/in";

        auto result = find_archive_name(dir, sets, ArchiveType::Textures);
        CHECK(result.has_value());
        CHECK(result->name == u8"dummy_sse"sv);
    }

    SECTION("returns nullopt if all possible file names in directory are taken")
    {
        const Path dir = "directory_with_all_possible_names_taken";
        auto result    = find_archive_name(dir, sets, ArchiveType::Textures);
        CHECK_FALSE(result.has_value());
    }
}