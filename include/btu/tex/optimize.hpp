/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/tex/detail/common.hpp"
#include "btu/tex/detail/formats.hpp"
#include "btu/tex/dimension.hpp"

#include <btu/common/games.hpp>

#include <variant>

namespace btu::tex {
class Texture;

struct Settings
{
    [[nodiscard]] static auto get(btu::common::Game game) noexcept -> const Settings &;

    btu::common::Game game;

    bool compress;
    std::variant<std::monostate, util::ResizeRatio, Dimension> resize;
    bool mipmaps;

    bool use_format_whitelist;
    DXGI_FORMAT output_format;
    std::vector<DXGI_FORMAT> allowed_formats;

    std::vector<std::u8string> landscape_textures;
};

struct OptimizationSteps
{
    std::optional<Dimension> resize;
    bool add_opaque_alpha = false;
    bool mipmaps;
    std::optional<DXGI_FORMAT> format;

    auto operator<=>(const OptimizationSteps &) const noexcept = default;
};

[[nodiscard]] auto optimize(Texture &&file, OptimizationSteps sets) noexcept -> Result;
[[nodiscard]] auto compute_optimization_steps(const Texture &file, const Settings &sets) noexcept
    -> OptimizationSteps;
} // namespace btu::tex
