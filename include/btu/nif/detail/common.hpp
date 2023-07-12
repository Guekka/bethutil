/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/error.hpp"
#include "btu/common/path.hpp"

#include <tl/expected.hpp>

namespace btu::nif {
using Error       = btu::common::Error;
using ResultError = tl::expected<void, Error>;

enum class HeadpartStatus
{
    Yes,
    No,
};
} // namespace btu::nif
