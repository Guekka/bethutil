/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/esp/error_code.hpp"
#include "utils.hpp"

#include <btu/bsa/archive.hpp>
#include <btu/bsa/unpack.hpp>
#include <btu/common/filesystem.hpp>

TEST_CASE("Unpack preview lists extraction and skipped paths without modifying files", "[src]")
{
    using namespace btu::bsa;

    const auto temp_dir        = TempPath(btu::fs::temp_directory_path() / "bsa_unpack_preview");
    const auto input_dir       = temp_dir.path() / "input";
    const auto output_dir      = temp_dir.path() / "output";
    const auto archive_path    = temp_dir.path() / "preview.bsa";
    const auto existing_target = output_dir / "Meshes" / "A.NIF";

    btu::fs::create_directories(input_dir / "meshes");
    create_file(input_dir / "meshes" / "a.nif", "archive-a");
    create_file(input_dir / "meshes" / "b.nif", "archive-b");

    auto archive = Archive{ArchiveVersion::sse, ArchiveType::Standard};
    auto file_a  = File{ArchiveVersion::sse, ArchiveType::Standard};
    REQUIRE(file_a.read(input_dir / "meshes" / "a.nif"));
    REQUIRE(archive.emplace("meshes/a.nif", std::move(file_a)));

    auto file_b = File{ArchiveVersion::sse, ArchiveType::Standard};
    REQUIRE(file_b.read(input_dir / "meshes" / "b.nif"));
    REQUIRE(archive.emplace("meshes/b.nif", std::move(file_b)));
    REQUIRE(std::move(archive).write(archive_path));

    btu::fs::create_directories(existing_target.parent_path());
    create_file(existing_target, "existing");
    const auto archive_before  = btu::common::read_file(archive_path);
    const auto existing_before = btu::common::read_file(existing_target);
    REQUIRE(archive_before.has_value());
    REQUIRE(existing_before.has_value());

    const auto preview = list_unpack_preview(UnpackSettings{.file_path                = archive_path,
                                                            .remove_arch              = true,
                                                            .overwrite_existing_files = false,
                                                            .extract_to_dir           = output_dir});

    REQUIRE(preview.has_value());
    CHECK(preview->files_to_extract == std::vector<Path>{"meshes/b.nif"});
    CHECK(preview->skipped_files == std::vector<Path>{"meshes/a.nif"});

    const auto archive_after  = btu::common::read_file(archive_path);
    const auto existing_after = btu::common::read_file(existing_target);
    REQUIRE(archive_after.has_value());
    REQUIRE(existing_after.has_value());
    CHECK(*archive_after == *archive_before);
    CHECK(*existing_after == *existing_before);
    CHECK_FALSE(btu::fs::exists(output_dir / "meshes" / "b.nif"));

    const auto overwrite_preview = list_unpack_preview(UnpackSettings{.file_path   = archive_path,
                                                                      .remove_arch = true,
                                                                      .overwrite_existing_files = true,
                                                                      .extract_to_dir = output_dir});

    REQUIRE(overwrite_preview.has_value());
    CHECK(overwrite_preview->files_to_extract == std::vector<Path>{"meshes/a.nif", "meshes/b.nif"});
    CHECK(overwrite_preview->skipped_files.empty());
    CHECK(btu::fs::exists(archive_path));
    CHECK_FALSE(btu::fs::exists(output_dir / "meshes" / "b.nif"));

    const auto actual_output_dir = temp_dir.path() / "actual_output";
    const auto actual_existing   = actual_output_dir / "Meshes" / "A.NIF";
    btu::fs::create_directories(actual_existing.parent_path());
    create_file(actual_existing, "existing");
    const auto actual_existing_before = btu::common::read_file(actual_existing);
    REQUIRE(actual_existing_before.has_value());

    const auto actual_preview = list_unpack_preview(UnpackSettings{.file_path                = archive_path,
                                                                   .overwrite_existing_files = false,
                                                                   .extract_to_dir           = actual_output_dir});
    REQUIRE(actual_preview.has_value());
    CHECK(actual_preview->files_to_extract == std::vector<Path>{"meshes/b.nif"});
    CHECK(actual_preview->skipped_files == std::vector<Path>{"meshes/a.nif"});

    const auto unpack_result = unpack(UnpackSettings{.file_path                = archive_path,
                                                     .overwrite_existing_files = false,
                                                     .extract_to_dir           = actual_output_dir});
    REQUIRE(unpack_result.has_value());

    const auto actual_existing_after = btu::common::read_file(actual_existing);
    REQUIRE(actual_existing_after.has_value());
    CHECK(*actual_existing_after == *actual_existing_before);
    const auto actual_relative_path = std::vector<Path>{"meshes/a.nif"};
    const auto existing_matches =
        btu::common::find_matching_paths_icase(actual_output_dir, actual_relative_path);
    REQUIRE(existing_matches.size() == 1);
    CHECK(btu::fs::exists(existing_matches.front()));
    const auto extracted_file = btu::common::read_file(actual_output_dir / "meshes" / "b.nif");
    const auto source_file    = btu::common::read_file(input_dir / "meshes" / "b.nif");
    REQUIRE(extracted_file.has_value());
    REQUIRE(source_file.has_value());
    CHECK(*extracted_file == *source_file);
}

TEST_CASE("Unpack preview propagates invalid archive errors without modifying files", "[src]")
{
    const auto temp_dir     = TempPath(btu::fs::temp_directory_path() / "bsa_unpack_preview_error");
    const auto archive_path = temp_dir.path() / "invalid.bsa";
    const auto output_dir   = temp_dir.path() / "output";
    btu::fs::create_directories(temp_dir.path());
    create_file(archive_path, "not a BSA archive");

    const auto preview = btu::bsa::list_unpack_preview(
        btu::bsa::UnpackSettings{.file_path = archive_path, .extract_to_dir = output_dir});

    CHECK_FALSE(preview.has_value());
    CHECK(btu::fs::exists(archive_path));
    CHECK_FALSE(btu::fs::exists(output_dir));
}

auto unpack_all(const Path &in, const Path &out, const btu::bsa::Settings &sets)
{
    auto archives = list_archive(in, sets);

    for (auto &&arch : archives)
    {
        const auto res = btu::bsa::unpack({.file_path = arch, .extract_to_dir = out});
        CHECK(res.has_value());
    }
}

TEST_CASE("unpack", "[src]")
{
    const Path dir = "bsa_unpack";
    const Path in  = dir / "in";
    btu::fs::remove_all(dir / "out");

    unpack_all(in, dir / "out", btu::bsa::Settings::get(btu::Game::SSE));
    unpack_all(in, dir / "out", btu::bsa::Settings::get(btu::Game::FO4));

    REQUIRE(btu::common::compare_directories(dir / "out", dir / "expected"));
}
