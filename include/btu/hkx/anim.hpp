/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/path.hpp"
#include "btu/hkx/detail/common.hpp"

namespace btu::hkx {
class AnimExe
{
    Path dir_;

public:
    static constexpr auto exe64to32 = "hkx64to32.exe";
    static constexpr auto exe32to64 = "hkx32to64.exe";

    static auto make(Path dir) noexcept -> tl::expected<AnimExe, Error>;
    auto get_directory() const noexcept -> const Path &;
};

class Anim
{
    Path path_;
    Path load_path_;

public:
    auto get() const noexcept -> const Path &;
    void set(Path path) noexcept;

    auto get_load_path() const noexcept -> const Path &;
    void set_load_path(Path path) noexcept;
};

[[nodiscard]] auto load(Path path, AnimExe exe) noexcept -> tl::expected<Anim, Error>;
[[nodiscard]] auto save(const Anim &anim, const Path &path) noexcept -> ResultError;
} // namespace btu::hkx
