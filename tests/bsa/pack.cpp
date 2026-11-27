/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"

#include <btu/bsa/unpack.hpp>

#include <algorithm>

TEST_CASE("Pack", "[src]")
{
    const Path dir = "pack";
    btu::fs::remove_all(dir / "output");
    auto test_pack = [&dir](auto game, auto name) {
        using namespace btu::bsa;

        const auto sets = Settings::get(game);

        const auto pack_settings = PackSettings{
            .input_dir     = dir / "input",
            .game_settings = sets,
            .compress      = Compression::Yes,
        };

        pack(pack_settings).for_each([name, &dir, &sets](Archive &&arch) {
            const auto type = arch.type();
            auto arch_name  = std::u8string(name)
                             + (type == ArchiveType::Textures ? u8" - Textures" : u8" - Main")
                             + sets.extension;

            const bool success = std::move(arch).write(dir / "output" / arch_name);
            REQUIRE(success);
        });
    };

    test_pack(btu::Game::FNV, u8"fnv");
    test_pack(btu::Game::SSE, u8"sse");
    test_pack(btu::Game::FO4, u8"fo4");
    test_pack(btu::Game::Starfield, u8"starfield");

    CHECK(btu::common::compare_directories(dir / "output", dir / "expected"));
}

TEST_CASE("Pack preview matches packed files", "[src]")
{
    using namespace btu::bsa;

    const auto temp_dir  = TempPath(btu::fs::temp_directory_path() / "bsa_pack_preview");
    const auto input_dir = temp_dir.path() / "input";
    btu::fs::create_directories(input_dir / "meshes");
    btu::fs::create_directories(input_dir / "textures" / "interface");
    btu::fs::create_directories(input_dir / "interface");
    btu::fs::create_directories(input_dir / "unknown");

    create_file(input_dir / "meshes" / "model.nif", "mesh");
    create_file(input_dir / "meshes" / "filtered.nif", "filtered");
    create_file(input_dir / "meshes" / "empty.nif");
    create_file(input_dir / "textures" / "interface" / "icon.dds", "texture");
    create_file(input_dir / "textures" / "landscape.png", "texture");
    create_file(input_dir / "interface" / "readme.txt", "text");
    create_file(input_dir / "unknown" / "ignored.bin", "ignored");
    create_file(input_dir / "root.nif", "root");

    const auto check_preview = [&](const btu::Game game, const bool has_texture_version) {
        auto game_settings = Settings::get(game);
        REQUIRE(game_settings.has_texture_version == has_texture_version);

        const auto pack_settings = PackSettings{
            .input_dir     = input_dir,
            .game_settings = game_settings,
            .compress      = Compression::No,
            .allow_file_pred =
                [](const Path &, const btu::fs::directory_entry &entry) {
                    return entry.path().filename() != "filtered.nif";
                },
        };

        auto preview = list_pack_preview(pack_settings);
        std::vector<Path> packed_standard;
        std::vector<Path> packed_textures;

        pack(pack_settings).for_each([&](Archive &&archive) {
            auto &packed_files = archive.type() == ArchiveType::Textures ? packed_textures : packed_standard;
            for (const auto &[path, _] : archive)
            {
                packed_files.emplace_back(path);
            }
        });

        auto sort_paths = [](std::vector<Path> &paths) { std::ranges::sort(paths); };
        sort_paths(preview.standard_files);
        sort_paths(preview.texture_files);
        sort_paths(packed_standard);
        sort_paths(packed_textures);

        auto expected_standard = std::vector<Path>{"meshes/model.nif", "interface/readme.txt"};
        auto expected_textures = std::vector<Path>{};
        if (has_texture_version)
        {
            expected_textures = {"textures/interface/icon.dds", "textures/landscape.png"};
        }
        else
        {
            expected_standard.emplace_back("textures/interface/icon.dds");
            expected_standard.emplace_back("textures/landscape.png");
        }
        sort_paths(expected_standard);
        sort_paths(expected_textures);

        CHECK(preview.standard_files == packed_standard);
        CHECK(preview.texture_files == packed_textures);
        CHECK(packed_standard == expected_standard);
        CHECK(packed_textures == expected_textures);
    };

    check_preview(btu::Game::SSE, true);
    check_preview(btu::Game::SLE, false);
}
