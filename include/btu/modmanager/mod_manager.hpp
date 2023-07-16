/* Copyright (C) 2020 - 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/modmanager/mod_folder.hpp"

namespace btu::modmanager {
[[maybe_unused]] constexpr auto k_force_process_folder = "ForceProcess.cao";

enum class ModManager
{
    Vortex,
    MO2,
    ManualForced,
    None
};
auto find_manager(const Path &dir) -> ModManager;

class ModsFolder
{
public:
    ModsFolder(Path root, std::u8string archive_ext);

    [[nodiscard]] auto to_flow()
    {
        return flow::from(folders_).map([this](auto &&f) { return ModFolder(f, archive_ext_); });
    }

private:
    Path root_;
    std::vector<Path> folders_;
    std::u8string archive_ext_;
};
} // namespace btu::modmanager
