/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <btu/common/error.hpp>
#include <btu/common/path.hpp>
#include <tl/expected.hpp>

#include <span>
#include <vector>

namespace btu::common {
[[nodiscard]] auto read_file(const Path &a_path) noexcept -> tl::expected<std::vector<std::byte>, Error>;

[[nodiscard]] auto write_file(const Path &a_path, std::span<const std::byte> data) noexcept
    -> tl::expected<void, Error>;

[[nodiscard]] auto compare_files(const Path &filename1, const Path &filename2) noexcept -> bool;

[[nodiscard]] auto compare_directories(const Path &dir1, const Path &dir2) noexcept -> bool;

[[nodiscard]] auto hard_link(const Path &from, const Path &to) noexcept -> tl::expected<void, Error>;

} // namespace btu::common
