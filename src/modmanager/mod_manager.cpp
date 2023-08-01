/* Copyright (C) 2020 - 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_manager.hpp"

#include <flux.hpp>

namespace btu::modmanager {
auto find_manager(const Path &dir) -> ModManager
{
    namespace fs = fs;

    /* Manual forced */
    if (fs::exists(dir / k_force_process_folder))
        return ModManager::ManualForced;

    /* Vortex */
    if (fs::exists(dir / "__vortex_staging_folder"))
        return ModManager::Vortex;

    /* MO2 */
    // Checking 10 dirs should be enough. One of them should be enough actually, but...better be safe
    const bool mo2 = flux::from_range(fs::directory_iterator(dir))
                         .filter([](auto &&entry) { return entry.is_directory(); })
                         .take(10)
                         .any([](auto &&dir) { return fs::exists(dir.path() / "meta.ini"); });
    if (mo2)
        return ModManager::MO2;

    // Kortex not yet handled

    return ModManager::None;
}

ModsFolder::ModsFolder(Path root, btu::bsa::Settings bsa_settings)
    : root_(std::move(root))
    , bsa_settings_(std::move(bsa_settings))
    , folders_(flux::from_range(fs::directory_iterator(root_))
                   .filter([](auto &&e) { return e.is_directory(); })
                   .map([](auto &&e) { return FLUX_FWD(e).path(); })
                   .to<std::vector>())
{
}

} // namespace btu::modmanager
