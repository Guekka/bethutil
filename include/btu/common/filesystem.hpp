/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/path.hpp"

#include <fstream>
#include <vector>

namespace btu::common {
namespace fs = std::filesystem;

[[nodiscard]] inline auto read_file(const Path &a_path) -> std::vector<std::byte>
{
    std::vector<std::byte> data;
    data.resize(fs::file_size(a_path));

    std::ifstream in{a_path, std::ios_base::in | std::ios_base::binary};
    in.exceptions(std::ios_base::failbit);
    in.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size())); // NOLINT

    return data;
}
} // namespace btu::common
