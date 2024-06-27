/* Copyright (C) 2019 - 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <btu/common/json.hpp>
#include <btu/tex/dxtex.hpp>

#include <array>
#include <string>

//Used to convert enum to string and vice versa

namespace btu::tex {
namespace detail {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): we need to stringify the enum
#define DEFFMT(fmt)                  \
    {                                \
        u8## #fmt, DXGI_FORMAT_##fmt \
    }

struct StringFormat
{
    std::u8string_view name;
    DXGI_FORMAT format;
};

constexpr std::array k_dxgi_formats = std::to_array<StringFormat>({
    // List does not include _TYPELESS or depth/stencil formats_string
    DEFFMT(R32G32B32A32_FLOAT),
    DEFFMT(R32G32B32A32_UINT),
    DEFFMT(R32G32B32A32_SINT),
    DEFFMT(R32G32B32_FLOAT),
    DEFFMT(R32G32B32_UINT),
    DEFFMT(R32G32B32_SINT),
    DEFFMT(R16G16B16A16_FLOAT),
    DEFFMT(R16G16B16A16_UNORM),
    DEFFMT(R16G16B16A16_UINT),
    DEFFMT(R16G16B16A16_SNORM),
    DEFFMT(R16G16B16A16_SINT),
    DEFFMT(R32G32_FLOAT),
    DEFFMT(R32G32_UINT),
    DEFFMT(R32G32_SINT),
    DEFFMT(R10G10B10A2_UNORM),
    DEFFMT(R10G10B10A2_UINT),
    DEFFMT(R11G11B10_FLOAT),
    DEFFMT(R8G8B8A8_UNORM),
    DEFFMT(R8G8B8A8_UNORM_SRGB),
    DEFFMT(R8G8B8A8_UINT),
    DEFFMT(R8G8B8A8_SNORM),
    DEFFMT(R8G8B8A8_SINT),
    DEFFMT(R16G16_FLOAT),
    DEFFMT(R16G16_UNORM),
    DEFFMT(R16G16_UINT),
    DEFFMT(R16G16_SNORM),
    DEFFMT(R16G16_SINT),
    DEFFMT(R32_FLOAT),
    DEFFMT(R32_UINT),
    DEFFMT(R32_SINT),
    DEFFMT(R8G8_UNORM),
    DEFFMT(R8G8_UINT),
    DEFFMT(R8G8_SNORM),
    DEFFMT(R8G8_SINT),
    DEFFMT(R16_FLOAT),
    DEFFMT(R16_UNORM),
    DEFFMT(R16_UINT),
    DEFFMT(R16_SNORM),
    DEFFMT(R16_SINT),
    DEFFMT(R8_UNORM),
    DEFFMT(R8_UINT),
    DEFFMT(R8_SNORM),
    DEFFMT(R8_SINT),
    DEFFMT(A8_UNORM),
    DEFFMT(R9G9B9E5_SHAREDEXP),
    DEFFMT(R8G8_B8G8_UNORM),
    DEFFMT(G8R8_G8B8_UNORM),
    DEFFMT(BC1_UNORM),
    DEFFMT(BC1_UNORM_SRGB),
    DEFFMT(BC2_UNORM),
    DEFFMT(BC2_UNORM_SRGB),
    DEFFMT(BC3_UNORM),
    DEFFMT(BC3_UNORM_SRGB),
    DEFFMT(BC4_UNORM),
    DEFFMT(BC4_SNORM),
    DEFFMT(BC5_UNORM),
    DEFFMT(BC5_SNORM),
    DEFFMT(B5G6R5_UNORM),
    DEFFMT(B5G5R5A1_UNORM),

    // DXGI 1.1 formats_string
    DEFFMT(B8G8R8A8_UNORM),
    DEFFMT(B8G8R8X8_UNORM),
    DEFFMT(R10G10B10_XR_BIAS_A2_UNORM),
    DEFFMT(B8G8R8A8_UNORM_SRGB),
    DEFFMT(B8G8R8X8_UNORM_SRGB),
    DEFFMT(BC6H_UF16),
    DEFFMT(BC6H_SF16),
    DEFFMT(BC7_UNORM),
    DEFFMT(BC7_UNORM_SRGB),

    // DXGI 1.2 formats_string
    DEFFMT(AYUV),
    DEFFMT(Y410),
    DEFFMT(Y416),
    DEFFMT(YUY2),
    DEFFMT(Y210),
    DEFFMT(Y216),
    // No support for legacy paletted video formats_string (AI44, IA44, P8, A8P8)
    DEFFMT(B4G4R4A4_UNORM),
});
#undef DEFMTT //cleanup
} // namespace detail

inline auto to_string(DXGI_FORMAT format) -> std::u8string_view
{
    // NOLINTNEXTLINE(readability-qualified-auto): gcc iterator is a pointer, not MSVC's iterator
    const auto it = std::ranges::find(detail::k_dxgi_formats, format, &detail::StringFormat::format);

    return it != detail::k_dxgi_formats.cend() ? it->name : u8"DXGI_FORMAT_UNKNOWN";
}

inline auto from_string(std::u8string_view str) -> DXGI_FORMAT
{
    // NOLINTNEXTLINE(readability-qualified-auto): see above
    const auto it = std::ranges::find(detail::k_dxgi_formats, str, &detail::StringFormat::name);

    return it != detail::k_dxgi_formats.cend() ? it->format : DXGI_FORMAT_UNKNOWN;
}

} // namespace btu::tex

template<>
struct nlohmann::adl_serializer<DXGI_FORMAT>
{
    static void to_json(json &j, const DXGI_FORMAT &format) { j = btu::tex::to_string(format); }

    static void from_json(const json &j, DXGI_FORMAT &format)
    {
        format = btu::tex::from_string(btu::common::as_utf8(j.get<std::string>()));
    }
}; // namespace nlohmann
