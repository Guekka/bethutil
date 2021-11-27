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

class Anim
{
    std::filesystem::path path_;
    std::filesystem::path exe_dir_;

    auto generate_name() const noexcept -> std::string;
    void get_name() const noexcept;

    auto sle_args() const noexcept -> std::vector<std::string>;
    auto sse_args() const noexcept -> std::vector<std::string>;

public:
    Anim(std::filesystem::path exe_dir)
        : exe_dir_(exe_dir)
    {
    }

    auto load(const std::filesystem::path &path) noexcept -> std::error_code;
    auto save(const std::filesystem::path &path) noexcept -> std::error_code;
    auto convert(btu::common::Game target_game) -> std::error_code;
};

} // namespace btu::hkx
