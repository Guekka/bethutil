/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/error_code.hpp"
#include "utils.hpp"

#include <btu/bsa/unpack.hpp>
#include <btu/common/filesystem.hpp>

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

TEST_CASE("unpack failure preserves archive when removal was requested")
{
    const Path dir      = "bsa_unpack";
    const auto archives = list_archive(dir / "in", btu::bsa::Settings::get(btu::Game::SSE));
    REQUIRE_FALSE(archives.empty());

    TempPath archive{dir, u8".bsa"};
    btu::fs::copy_file(archives.front(), archive.path());

    TempPath blocked_output{dir};
    create_file(blocked_output.path());

    const auto result = btu::bsa::unpack(
        {.file_path = archive.path(), .remove_arch = true, .extract_to_dir = blocked_output.path()});
    REQUIRE_FALSE(result);
    CHECK(result.error() == btu::bsa::BsaErr::FailedToWriteFile);
    CHECK(result.error().message() == "failed to write file");
    CHECK(btu::fs::exists(archive.path()));
}
