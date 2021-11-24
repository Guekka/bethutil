/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <btu/common/error.hpp>
#include <btu/common/games.hpp>

#include <filesystem>
#include <iostream>

namespace btu::hkx {

auto load(const std::filesystem::path &path, const std::filesystem::path &exe_dir) noexcept
    -> std::error_code;
auto save(const std::filesystem::path &path, const std::filesystem::path &exe_dir) noexcept
    -> std::error_code;

auto convert(btu::common::Game target_game, const std::filesystem::path &exe_dir) -> std::error_code;

} // namespace btu::hkx
