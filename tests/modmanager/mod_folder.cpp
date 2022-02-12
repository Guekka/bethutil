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
    fs::remove_all(dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "input", u8".ba2");
    flow::from(mf).for_each([&](auto &&f) {
        const auto out = dir / "output" / f.get_relative_path();
        fs::create_directories(out.parent_path());
        f.load();
        f.write(out);
    });
    REQUIRE(btu::common::compare_directories(dir / "output", dir / "expected"));
}

TEST_CASE("ModFolder reintegrate", "[src]")
{
    const Path dir = "modfolder_reintegrate";
    // operate on copy
    fs::remove_all(dir / "output");
    fs::copy(dir / "input", dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "output", u8".ba2");
    // Change one byte in each file
    flow::from(mf).for_each([&](auto &&f) {
        f.load();
        binary_io::any_ostream buffer{binary_io::memory_ostream{}};
        f.write(buffer);
        auto &data  = buffer.get<binary_io::memory_ostream>().rdbuf();
        data.back() = std::byte{'0'}; // Change one byte
        f.read(data);
    });
    mf.reintegrate();

    REQUIRE(btu::common::compare_directories(dir / "output", dir / "expected"));
}
