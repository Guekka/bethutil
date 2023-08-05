/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include <btu/bsa/settings.hpp>
#include <flux.hpp>

#include <functional>

namespace btu::bsa {
using AllowFilePred = std::function<bool(const Path &dir, fs::directory_entry const &file_info)>;

struct PackSettings
{
    Path input_dir;
    Settings game_settings;

    Compression compress = Compression::Yes;

    std::optional<AllowFilePred> allow_file_pred = std::nullopt;
};

[[nodiscard]] auto pack(PackSettings settings) noexcept -> flux::generator<bsa::Archive &&>;

} // namespace btu::bsa
