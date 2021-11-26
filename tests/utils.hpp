/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include "btu/bsa/archive_data.hpp"
#include "btu/bsa/detail/common.hpp"
#include "catch2/catch.hpp"

#include <sstream>

using namespace btu::bsa;
namespace Catch {

template<>
struct StringMaker<ArchiveData>
{
    static std::string convert(const ArchiveData &in)
    {
        std::stringstream os;
        os << "ArchiveData{"
           << "size:{" << in.size().compressed << ", " << in.size().uncompressed
           << "max_size: " << in.max_size() << "type: " << btu::common::to_underlying(in.type());
        return os.str();
    }
};

template<>
struct StringMaker<Path>
{
    static std::string convert(const Path &in) { return in.string(); }
};

} // namespace Catch
