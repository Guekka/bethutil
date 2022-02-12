/* Copyright (C) 2020 - 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_manager.hpp"

#include <flow.hpp>

/*
std::optional<QString> GeneralSettings::isValid() const
{
      const ModManager managedBy = findManager(inputDir);

    if (eMode() == SeveralMods && managedBy == ModManager::None)
    {
        return tr("'Several mods' mode is enabled, but this path does not seem to be handled by a mod "
                  "manager.\n If you are sure you want to process this folder, please create a file "
                  "named '%1' in this folder. This is a necessary evil to ensure safety for your mods.\n"
                  "Path: '%2'")
            .arg(forceProcessFolder, sInputPath());
    }

    const bool isSingleModReady = managedBy == ModManager::None || managedBy == ModManager::ManualForced;
    if (eMode() == SingleMod && !isSingleModReady)
    {
        return tr("'Single mod' mode is enabled, but this path seems to be handled by a mod manager. "
                  "Path: '%1'")
            .arg(sInputPath());
    }
}
*/

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
    //Checking 10 dirs should be enough. One of them should be enough actually, but...better be safe
    const bool mo2 = flow::from(fs::directory_iterator(dir))
                         .filter([](auto &&entry) { return entry.is_directory(); })
                         .take(10)
                         .any([](auto &&dir) { return fs::exists(dir.path() / "meta.ini"); });
    if (mo2)
        return ModManager::MO2;

    //Kortex not yet handled

    return ModManager::None;
}

ModsFolder::ModsFolder(Path root, std::u8string archive_ext)
    : root_(std::move(root))
    , folders_(flow::from(fs::directory_iterator(root_))
                   .filter([](auto &&e) { return e.is_directory(); })
                   .map([](auto &&e) { return FLOW_FWD(e).path(); })
                   .to_vector())
    , archive_ext_(std::move(archive_ext))
{
}

} // namespace btu::modmanager
