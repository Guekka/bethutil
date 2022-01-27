/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_manager.hpp"

#include "../utils.hpp"

TEST_CASE("find_manager", "[src]")
{
    using btu::modmanager::find_manager;
    using btu::modmanager::ModManager;

    const Path dir = "mod_manager";
    REQUIRE(find_manager(dir / "vortex") == ModManager::Vortex);
    REQUIRE(find_manager(dir / "forced") == ModManager::ManualForced);
    REQUIRE(find_manager(dir / "mo2") == ModManager::MO2);
    REQUIRE(find_manager(dir / "none") == ModManager::None);
}
