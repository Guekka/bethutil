/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <btu/common/error.hpp>
#include <btu/common/games.hpp>
#include <btu/common/path.hpp>
#include <tl/expected.hpp>

#include <array>

namespace btu::hkx {
using btu::common::Error;

using ResultError = tl::expected<void, Error>;

namespace detail {
class AnimExeInfo
{
public:
    AnimExeInfo() = default;

    AnimExeInfo(const AnimExeInfo &)                     = delete;
    AnimExeInfo(AnimExeInfo &&)                          = delete;
    auto operator=(const AnimExeInfo &) -> AnimExeInfo & = delete;
    auto operator=(AnimExeInfo &&) -> AnimExeInfo      & = delete;

    virtual ~AnimExeInfo() = default;

    [[nodiscard]] virtual constexpr auto name() const noexcept -> std::string_view             = 0;
    [[nodiscard]] virtual constexpr auto input_file_name() const noexcept -> std::string_view  = 0;
    [[nodiscard]] virtual constexpr auto output_file_name() const noexcept -> std::string_view = 0;
    [[nodiscard]] virtual constexpr auto target_game() const noexcept -> btu::Game             = 0;

    [[nodiscard]] virtual auto get_required_files(const Path &exe_dir) const noexcept
        -> std::vector<Path>                                                                        = 0;
    [[nodiscard]] virtual auto get_full_args(const Path &exe_dir) const -> std::vector<std::string> = 0;
};

using AnimExeRef = std::reference_wrapper<const AnimExeInfo>;

} // namespace detail

class AnimExe
{
public:
    [[nodiscard]] static auto make(Path exe_dir) noexcept -> tl::expected<AnimExe, Error>;

    [[nodiscard]] auto convert(btu::Game target_game, const Path &input, const Path &output) const
        -> ResultError;

    [[nodiscard]] auto convert(btu::Game target_game, std::span<std::byte> input) const
        -> tl::expected<std::vector<std::byte>, Error>;

private:
    Path exe_dir_;
    std::vector<detail::AnimExeRef> detected_;

    using CopyInput = std::function<ResultError(const Path &input_path)>;

    [[nodiscard]] auto convert_impl(btu::Game target_game, const CopyInput &copy_input) const noexcept
        -> tl::expected<Path, Error>;

    AnimExe(Path exe_dir, std::vector<detail::AnimExeRef> detected) noexcept;
};
} // namespace btu::hkx
