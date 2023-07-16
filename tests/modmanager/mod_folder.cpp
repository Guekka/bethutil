/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/memory_stream.hpp>

TEST_CASE("ModFolder", "[src]")
{
    const Path dir = "modfolder";
    btu::fs::remove_all(dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "input", u8".ba2");
    mf.iterate([&](btu::modmanager::ModFolder::ModFile &&f) {
        const auto out = dir / "output" / f.relative_path;
        btu::fs::create_directories(out.parent_path());
        btu::common::write_file(out, f.content);
    });
    REQUIRE(btu::common::compare_directories(dir / "output", dir / "expected"));
}

TEST_CASE("ModFolder reintegrate", "[src]")
{
    const Path dir = "modfolder_reintegrate";
    // operate on copy
    btu::fs::remove_all(dir / "output");
    btu::fs::copy(dir / "input", dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "output", u8".ba2");
    // Change one byte in each file
    mf.transform([](btu::modmanager::ModFolder::ModFile &&f) {
        f.content.back() = std::byte{'0'}; // Change one byte
        return std::make_optional(std::move(f.content));
    });

    REQUIRE(btu::common::compare_directories(dir / "output", dir / "expected"));
}

TEST_CASE("ModFolder size", "[src]")
{
    const Path dir = "modfolder";
    auto mf        = btu::modmanager::ModFolder(dir / "input", u8".ba2");
    REQUIRE(mf.size() == 4);
}
