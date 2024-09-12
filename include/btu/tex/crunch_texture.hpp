/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <btu/common/path.hpp>
#include <btu/tex/detail/common.hpp>
#include <btu/tex/dxtex.hpp> // for DXGI_FORMAT

#include <crunch/crn_mipmapped_texture.h>

#include <filesystem>
#include <span>

namespace crnlib {
class mipmapped_texture;

auto operator==(const image_u8 &lhs, const image_u8 &rhs) noexcept -> bool;
auto operator==(const mipmapped_texture &lhs, const mipmapped_texture &rhs) noexcept -> bool;
} // namespace crnlib

namespace btu::tex {
using crnlib::mipmapped_texture;

struct Dimension;
enum class TextureType : std::uint8_t;

class CrunchTexture
{
public:
    void set(const mipmapped_texture &tex) noexcept;

    [[nodiscard]] auto get() noexcept -> mipmapped_texture &;
    [[nodiscard]] auto get() const noexcept -> const mipmapped_texture &;

    [[nodiscard]] auto get_dimension() const noexcept -> Dimension;

    [[nodiscard]] auto get_load_path() const noexcept -> const Path &;
    void set_load_path(Path path) noexcept;

    [[nodiscard]] auto get_texture_type() const noexcept -> TextureType;
    [[nodiscard]] auto get_format_as_dxgi() const noexcept -> DXGI_FORMAT;

    [[nodiscard]] auto operator==(const CrunchTexture &) const noexcept -> bool = default;

private:
    Path load_path_;
    mipmapped_texture tex_;
};

[[nodiscard]] auto load_crunch(Path path) noexcept -> tl::expected<CrunchTexture, Error>;
[[nodiscard]] auto load_crunch(Path relative_path,
                               std::span<const std::byte> data) noexcept -> tl::expected<CrunchTexture, Error>;

[[nodiscard]] auto save(const CrunchTexture &tex, const Path &path) noexcept -> ResultError;
[[nodiscard]] auto save(const CrunchTexture &tex) noexcept -> tl::expected<std::vector<std::byte>, Error>;
} // namespace btu::tex
