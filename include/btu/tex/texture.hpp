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

namespace DirectX { // NOLINT
struct Image;
class ScratchImage;
struct TexMetadata;

auto operator==(const ScratchImage &lhs, const ScratchImage &rhs) noexcept -> bool;
auto operator==(const TexMetadata &lhs, const TexMetadata &rhs) noexcept -> bool;
} // namespace DirectX

namespace btu::tex {
using DirectX::Image;
using DirectX::ScratchImage;
using DirectX::TexMetadata; // NOLINT Not actually unused

struct Dimension;

namespace detail {
constexpr size_t k_sizeof_scratchimage  = 88;
constexpr size_t k_alignof_scratchimage = 8;

class ScratchImagePimpl
{
public:
    ScratchImagePimpl() noexcept(false);

    ScratchImagePimpl(const ScratchImagePimpl &) = delete;
    ScratchImagePimpl(ScratchImagePimpl &&other) noexcept;

    auto operator=(const ScratchImagePimpl &) -> ScratchImagePimpl & = delete;
    auto operator=(ScratchImagePimpl &&other) noexcept -> ScratchImagePimpl &;

    ~ScratchImagePimpl();

    auto get() &noexcept -> ScratchImage &;
    auto get() &&noexcept -> ScratchImage;
    auto get() const &noexcept -> const ScratchImage &;

private:
    std::aligned_storage_t<k_sizeof_scratchimage, k_alignof_scratchimage> storage_;
};

auto operator==(const ScratchImagePimpl &lhs, const ScratchImagePimpl &rhs) noexcept -> bool;
} // namespace detail

[[maybe_unused]] constexpr auto canonize_path = btu::common::make_path_canonizer(u8"textures/");

class Texture
{
public:
    void set(ScratchImage &&tex) noexcept;

    auto get() noexcept -> ScratchImage &;
    auto get() const noexcept -> const ScratchImage &;

    auto get_images() const noexcept -> std::span<const Image>;

    auto get_dimension() const noexcept -> Dimension;

    auto get_load_path() const noexcept -> const Path &;
    void set_load_path(Path path) noexcept;

    auto operator==(const Texture &) const noexcept -> bool = default;

private:
    Path load_path_;
    detail::ScratchImagePimpl tex_;
};

[[nodiscard]] auto load(Path path) noexcept -> tl::expected<Texture, Error>;
[[nodiscard]] auto save(const Texture &tex, Path path) noexcept -> ResultError;

} // namespace btu::tex
