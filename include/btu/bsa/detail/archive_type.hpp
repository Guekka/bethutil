/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>

namespace btu::bsa {
enum class ArchiveType : std::uint8_t
{
    Textures,
    Standard,
};

// Maps to rsm-bsa values. Enforced by static_assert in source file.
// We avoid including rsm-bsa in public headers.
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

NLOHMANN_JSON_SERIALIZE_ENUM(ArchiveType,
                             {{ArchiveType::Textures, "textures"}, {ArchiveType::Standard, "standard"}})

NLOHMANN_JSON_SERIALIZE_ENUM(ArchiveVersion,
                             {{ArchiveVersion::tes3, "tes3"},
                              {ArchiveVersion::tes4, "tes4"},
                              {ArchiveVersion::fo3, "fo3"},
                              {ArchiveVersion::tes5, "tes5"},
                              {ArchiveVersion::sse, "sse"},
                              {ArchiveVersion::fo4, "fo4"},
                              {ArchiveVersion::fo4dx, "fo4dx"}})

} // namespace btu::bsa
