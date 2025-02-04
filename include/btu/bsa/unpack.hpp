/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include <btu/common/error.hpp>
#include <btu/common/filesystem.hpp>

namespace btu::bsa {

struct UnpackSettings
{
    Path file_path;
    bool remove_arch                   = false;
    bool overwrite_existing_files      = false;
    std::optional<Path> extract_to_dir = std::nullopt;
};

[[nodiscard]] auto unpack(UnpackSettings sets) -> tl::expected<void, common::Error>;

} // namespace btu::bsa
