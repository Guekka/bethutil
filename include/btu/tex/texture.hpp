/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/tex/detail/common.hpp"

#include <filesystem>
#include <type_traits>

namespace DirectX { // NOLINT
class ScratchImage;
class TexMetadata;

auto operator==(const ScratchImage &lhs, const ScratchImage &rhs) noexcept -> bool;
auto operator==(const TexMetadata &lhs, const TexMetadata &rhs) noexcept -> bool;
} // namespace DirectX

namespace btu::tex {
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

class Texture
{
public:
    [[nodiscard]] auto load_file(std::filesystem::path path) noexcept -> ResultError;
    [[nodiscard]] auto save_file(std::filesystem::path path) const noexcept -> ResultError;

    auto set(ScratchImage &&tex) noexcept -> void;

    auto get() noexcept -> ScratchImage &;
    auto get() const noexcept -> const ScratchImage &;

    auto get_dimension() const noexcept -> Dimension;

    auto operator==(const Texture &) const noexcept -> bool = default;

private:
    std::filesystem::path load_path_;
    detail::ScratchImagePimpl tex_;
};
} // namespace btu::tex
