/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <bsa/bsa.hpp>

namespace btu::bsa {
enum class ArchiveType
{
    Textures,
    Standard,
    Incompressible
};

enum class ArchiveVersion : std::uint32_t
{
    tes3  = 1,
    tes4  = ::bsa::detail::to_underlying(::bsa::tes4::version::tes4),
    fo3   = ::bsa::detail::to_underlying(::bsa::tes4::version::fo3),
    tes5  = ::bsa::detail::to_underlying(::bsa::tes4::version::tes5),
    sse   = ::bsa::detail::to_underlying(::bsa::tes4::version::sse),
    fo4   = ::bsa::detail::to_underlying(::bsa::fo4::format::general),
    fo4dx = ::bsa::detail::to_underlying(::bsa::fo4::format::directx),
};
} // namespace btu::bsa
