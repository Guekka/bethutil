/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include <btu/bsa/settings.hpp>

#include <functional>

namespace btu::bsa {
using AllowFilePred  = std::function<bool(const Path &dir, fs::directory_entry const &file_info)>;
using ArchiveNameGen = std::function<std::u8string(ArchiveType)>;

struct PackSettings
{
    Path input_dir;
    Path output_dir;

    Settings game_settings;

    Compression compress = Compression::Yes;

    ArchiveNameGen archive_name_gen;
    std::optional<AllowFilePred> allow_file_pred = std::nullopt;
};

/// \return the list of files which failed to pack
[[nodiscard]] auto pack(const PackSettings &settings) noexcept
    -> std::vector<std::pair<Path, std::exception_ptr>>;

} // namespace btu::bsa
