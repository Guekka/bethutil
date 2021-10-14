/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>

namespace btu::bsa {
enum class ArchiveType
{
    Textures,
    Standard,
    Incompressible
};

enum class ArchiveVersion : std::uint32_t
{
    tes3  = 1,
    tes4  = 103,
    fo3   = 104,
    tes5  = 104,
    sse   = 105,
    fo4   = 1280462407,
    fo4dx = 808540228,
};
} // namespace btu::bsa
