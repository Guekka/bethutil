/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/memory_stream.hpp>

auto archive_too_large_handler(const btu::Path & /*unused*/,
                               const btu::modmanager::ModFolder::ArchiveTooLargeState & /*unused*/)
{
    FAIL("Archive too large, should not happen in tests");
    return btu::modmanager::ModFolder::ArchiveTooLargeAction::Skip;
}

TEST_CASE("ModFolder", "[src]")
{
    const Path dir = "modfolder";
    btu::fs::remove_all(dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "input", btu::bsa::Settings::get(btu::Game::FO4));
    mf.iterate(
        [&](btu::modmanager::ModFolder::ModFile &&f) {
            const auto out = dir / "output" / f.relative_path;
            btu::fs::create_directories(out.parent_path());
            btu::common::write_file(out, std::move(*f.content));
        },
        archive_too_large_handler);
    REQUIRE(btu::common::compare_directories(dir / "output", dir / "expected"));
}

TEST_CASE("ModFolder transform", "[src]")
{
    const Path dir = "modfolder_transform";
    // operate on copy
    btu::fs::remove_all(dir / "output");
    btu::fs::copy(dir / "input", dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "output", btu::bsa::Settings::get(btu::Game::SSE));
    // Change one byte in each file
    mf.transform(
        [](btu::modmanager::ModFolder::ModFile &&f) {
            f.content->back() = std::byte{'0'}; // Change one byte
            return std::make_optional(*std::move(f.content));
        },
        archive_too_large_handler);

    REQUIRE(btu::common::compare_directories(dir / "output", dir / "expected"));
}

TEST_CASE("ModFolder size", "[src]")
{
    const Path dir = "modfolder";
    auto mf        = btu::modmanager::ModFolder(dir / "input", btu::bsa::Settings::get(btu::Game::FO4));
    REQUIRE(mf.size() == 4);
}

TEST_CASE("Iterate mod folder with archive too big")
{
    const Path dir = "modfolder";
    auto sets      = btu::bsa::Settings::get(btu::Game::FO4);
    sets.max_size  = 1;

    bool called = false;

    auto mf = btu::modmanager::ModFolder(dir / "input", sets);
    mf.iterate([](const btu::modmanager::ModFolder::ModFile & /*unused*/) {},
               [&](const btu::Path & /*unused*/,
                   const btu::modmanager::ModFolder::ArchiveTooLargeState &state) {
                   CHECK(state == btu::modmanager::ModFolder::ArchiveTooLargeState::BeforeProcessing);
                   called = true;
                   return btu::modmanager::ModFolder::ArchiveTooLargeAction::Skip;
               });

    REQUIRE(called);
}
