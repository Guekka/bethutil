/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/bsa/settings.hpp"

namespace btu::bsa {
/**
 * @brief Find a name for the archive. Try plugin name, then directory name, then random name
 *
 * @param directory The directory that will contain the archive
 * @param sets Game settings
 * @param type Archive type
 * @return An archive name,or std::nullopt if you are particularly unlucky
 */
[[nodiscard]] auto find_archive_name(const Path &directory,
                                     const Settings &sets,
                                     ArchiveType type) noexcept -> std::optional<Path>;

[[nodiscard]] auto list_archive(const Path &dir, const Settings &sets) noexcept -> std::vector<Path>;
[[nodiscard]] auto list_plugins(const Path &dir, const Settings &sets) noexcept -> std::vector<Path>;

void clean_dummy_plugins(std::span<const Path> plugins, const Settings &sets);
void make_dummy_plugins(std::span<const Path> archives, const Settings &sets);

/// Cleans dummy plugins and make new ones
void remake_dummy_plugins(const Path &directory, const Settings &sets);
} // namespace btu::bsa
