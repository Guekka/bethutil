/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "../utils.hpp"
#include "btu/bsa/archive_data.hpp"
#include "catch.hpp"

#include <sstream>

namespace Catch {

template<>
struct StringMaker<btu::bsa::ArchiveData>
{
    static auto convert(const btu::bsa::ArchiveData &in) -> std::string
    {
        std::stringstream os;
        os << "ArchiveData{"
           << "size: " << in.size() << ", "
           << "max_size: " << in.max_size() << "type: " << btu::common::to_underlying(in.type());
        return os.str();
    }
};
} // namespace Catch
