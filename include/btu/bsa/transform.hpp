/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

/*
#include "archive_data.hpp"
#include "common.hpp"
#include "detail/BSACallback.hpp"
#include "detail/plugin.hpp"
#include "detail/settings.hpp"
#include "transform_archive.hpp"

namespace BethUtil::BSA {
inline void transform(const Path& file,
                      const Path& output,
                      libbsarch::transform_callback const& callback,
                      Settings const& sets)
{
    const auto path = FilePath::make(file, sets, FileTypes::BSA);
    auto format     = sets.format;
    if (path->suffix_ == sets.textureSuffix)
        format = *sets.textureFormat;

    libbsarch::bsa bsa;

    bsa.set_dds_callback(&BSACallback, file);
    bsa.load(file);

    libbsarch::transform_archive(bsa, output, callback, format);
}
} // namespace BethUtil::BSA
*/
