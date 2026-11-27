/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include <btu/bsa/settings.hpp>
#include <flux.hpp>

#include <functional>
#include <vector>

namespace btu::bsa {
using AllowFilePred = std::function<bool(const Path &dir, fs::directory_entry const &file_info)>;

struct PackSettings
{
    Path input_dir;
    Settings game_settings;

    Compression compress = Compression::Yes;

    std::optional<AllowFilePred> allow_file_pred = std::nullopt;
};

struct PackPreview
{
    /// Paths relative to PackSettings::input_dir.
    std::vector<Path> standard_files;
    /// Paths relative to PackSettings::input_dir; empty if no texture archives are supported.
    std::vector<Path> texture_files;
};

/// Lists files selected by pack() without reading file content or compressing.
/// The paths are relative to PackSettings::input_dir. This does not predict archive
/// splitting, which depends on post-compression file sizes.
[[nodiscard]] auto list_pack_preview(const PackSettings &settings) noexcept -> PackPreview;

[[nodiscard]] auto pack(PackSettings settings) noexcept -> flux::generator<Archive &&>;

} // namespace btu::bsa
