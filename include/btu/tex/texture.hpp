/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/path.hpp"
#include "btu/tex/detail/common.hpp"

#include <filesystem>
#include <span>
#include <type_traits>

namespace DirectX { // NOLINT(readability-identifier-naming)
struct Image;
class ScratchImage;
struct TexMetadata;

auto operator==(const ScratchImage &lhs, const ScratchImage &rhs) noexcept -> bool;
auto operator==(const TexMetadata &lhs, const TexMetadata &rhs) noexcept -> bool;
} // namespace DirectX

namespace btu::tex {
using DirectX::Image;
using DirectX::ScratchImage;
using DirectX::TexMetadata;

struct Dimension;

[[maybe_unused]] constexpr auto canonize_path = btu::common::make_path_canonizer(u8"textures/");

class Texture
{
public:
    Texture();

    void set(ScratchImage &&tex) noexcept;

    [[nodiscard]] auto get() noexcept -> ScratchImage &;
    [[nodiscard]] auto get() const noexcept -> const ScratchImage &;

    [[nodiscard]] auto get_images() const noexcept -> std::span<const Image>;

    [[nodiscard]] auto get_dimension() const noexcept -> Dimension;

    [[nodiscard]] auto get_load_path() const noexcept -> const Path &;
    void set_load_path(Path path) noexcept;

    [[nodiscard]] auto operator==(const Texture &) const noexcept -> bool = default;

private:
    Path load_path_;
    ScratchImage tex_;
};

[[nodiscard]] auto load(Path path) noexcept -> tl::expected<Texture, Error>;
[[nodiscard]] auto load(Path relative_path,
                        std::span<std::byte> data) noexcept -> tl::expected<Texture, Error>;

[[nodiscard]] auto save(const Texture &tex, const Path &path) noexcept -> ResultError;
[[nodiscard]] auto save(const Texture &tex) noexcept -> tl::expected<std::vector<std::byte>, Error>;

} // namespace btu::tex
