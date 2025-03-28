/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/tex/error_code.hpp"

#include <tl/expected.hpp>

namespace btu::tex {
class Texture;
class CrunchTexture;

using Result       = tl::expected<Texture, Error>;
using ResultCrunch = tl::expected<CrunchTexture, Error>;
using ResultError  = tl::expected<void, Error>;
} // namespace btu::tex
