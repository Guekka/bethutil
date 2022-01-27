/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"

TEST_CASE("ModFolder", "[src]")
{
    const Path dir = "modfolder";
    std::filesystem::remove_all(dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "input", u8".ba2");
    flow::from(mf).for_each([&](auto &&f) {
        const auto out = dir / "output" / f.get_relative_path();
        std::filesystem::create_directories(out.parent_path());
        f.write(out);
    });
    REQUIRE(btu::common::compare_directories(dir / "output", dir / "expected"));
    std::filesystem::remove_all(dir / "output");
}
