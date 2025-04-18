/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/tex/detail/common.hpp"
#include "btu/tex/dimension.hpp"
#include "btu/tex/formats.hpp"
#include "compression_device.hpp"

#include <btu/common/games.hpp>
#include <btu/common/json.hpp>

#include <variant>

namespace btu::tex {
class Texture;
class CrunchTexture;

struct Settings
{
    [[nodiscard]] static auto get(Game game) noexcept -> const Settings &;

    Game game;

    bool compress;
    std::variant<std::monostate, util::ResizeRatio, Dimension> resize;
    bool mipmaps;

    bool use_format_whitelist;
    std::vector<DXGI_FORMAT> allowed_formats;

    BestFormatFor output_format;

    std::vector<std::u8string> landscape_textures;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Settings,
                                   game,
                                   compress,
                                   resize,
                                   mipmaps,
                                   use_format_whitelist,
                                   allowed_formats,
                                   output_format,
                                   landscape_textures)

struct OptimizationSteps
{
    std::optional<Dimension> resize;
    bool add_transparent_alpha = false;
    bool mipmaps               = false;
    DXGI_FORMAT best_format    = DXGI_FORMAT_UNKNOWN;
    bool convert               = false;

    auto operator<=>(const OptimizationSteps &) const noexcept = default;
};

[[nodiscard]] auto optimize(Texture &&file, OptimizationSteps sets, CompressionDevice &dev) noexcept -> Result;
[[nodiscard]] auto optimize(CrunchTexture &&file,
                            OptimizationSteps sets,
                            CompressionDevice &dev) noexcept -> ResultCrunch;
[[nodiscard]] auto compute_optimization_steps(const Texture &file,
                                              const Settings &sets) noexcept -> OptimizationSteps;
[[nodiscard]] auto compute_optimization_steps(const CrunchTexture &file,
                                              const Settings &sets) noexcept -> OptimizationSteps;
} // namespace btu::tex
