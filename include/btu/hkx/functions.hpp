/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/path.hpp"

#include <btu/common/error.hpp>
#include <btu/common/games.hpp>

#include <filesystem>

namespace btu::hkx {

class Anim
{
    Path path_;
    Path exe_dir_;

    auto generate_name() const noexcept -> std::string;
    void get_name() const noexcept;

    auto sle_args() const noexcept -> std::vector<std::string>;
    auto sse_args() const noexcept -> std::vector<std::string>;

public:
    Anim(Path exe_dir)
        : exe_dir_(exe_dir)
    {
    }

    auto load(const Path &path) noexcept -> std::error_code;
    auto save(const Path &path) noexcept -> std::error_code;
    auto convert(btu::Game target_game) -> std::error_code;
};

} // namespace btu::hkx
