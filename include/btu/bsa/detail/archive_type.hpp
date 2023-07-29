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

/**
 * @brief Retrieves the archive type based on the given version.
 *
 * This function determines the archive type based on the provided @c version.
 * The archive type is returned as a result.
 *
 * @param version The ArchiveVersion object representing the version.
 *
 * @return The ArchiveType associated with the specified version.
 */
[[nodiscard]] inline auto type_from_version(ArchiveVersion version) noexcept -> ArchiveType
{
    switch (version)
    {
        case ArchiveVersion::tes3:
        case ArchiveVersion::tes4:
        case ArchiveVersion::fo3:
        case ArchiveVersion::sse:
        case ArchiveVersion::fo4: return ArchiveType::Standard;
        case ArchiveVersion::fo4dx: return ArchiveType::Textures;
    }
}
} // namespace btu::bsa
