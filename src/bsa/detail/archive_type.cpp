/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/detail/archive_type.hpp"

#include "btu/common/metaprogramming.hpp"

#include <bsa/bsa.hpp>

// Avoid including rsm-bsa in public header
namespace btu::bsa {

using btu::common::to_underlying;

static_assert(to_underlying(ArchiveVersion::tes3) == 1);
static_assert(to_underlying(ArchiveVersion::tes4) == to_underlying(::bsa::tes4::version::tes4));
static_assert(to_underlying(ArchiveVersion::fo3) == to_underlying(::bsa::tes4::version::fo3));
static_assert(to_underlying(ArchiveVersion::tes5) == to_underlying(::bsa::tes4::version::tes5));
static_assert(to_underlying(ArchiveVersion::sse) == to_underlying(::bsa::tes4::version::sse));
static_assert(to_underlying(ArchiveVersion::fo4) == to_underlying(::bsa::fo4::format::general));
static_assert(to_underlying(ArchiveVersion::fo4dx) == to_underlying(::bsa::fo4::format::directx));

} // namespace btu::bsa
