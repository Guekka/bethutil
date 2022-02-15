/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/path.hpp"
#include "btu/hkx/detail/common.hpp"

#include <btu/common/error.hpp>
#include <btu/common/games.hpp>

#include <filesystem>

namespace btu::hkx {
class Anim;
class AnimExe;

[[nodiscard]] auto convert(Anim &anim, const AnimExe &exe, btu::Game target_game) -> ResultError;

} // namespace btu::hkx
