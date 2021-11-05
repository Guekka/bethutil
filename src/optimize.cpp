/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/optimize.hpp"

#include "btu/tex/compression_device.hpp"
#include "btu/tex/functions.hpp"

#include <DirectXTex.h>
#include <btu/common/algorithms.hpp>

namespace btu::tex {
auto optimize(Texture &&file, OptimizationSteps sets) noexcept -> Result
{
// Idea from https://vector-of-bool.github.io/2021/04/20/terse-lambda-macro.html
#define F(FUNC) [&](Texture &&f) { return FUNC; }

    auto dev              = CompressionDevice::make(0).value();
    const auto compressed = DirectX::IsCompressed(file.get().GetMetadata().format);
    return Result{std::move(file)}
        .and_then(F(compressed ? decompress(std::move(f)) : std::move(f)))
        .and_then(F(sets.resize ? resize(std::move(f), sets.resize.value()) : std::move(f)))
        .and_then(F(sets.add_opaque_alpha ? make_opaque_alpha(std::move(f)) : std::move(f)))
        .and_then(F(sets.mipmaps ? generate_mipmaps(std::move(f)) : std::move(f)))
        .and_then(F(sets.format ? convert(std::move(f), sets.format.value(), dev) : std::move(f)));

#undef F
}

/// SSE landscape textures are way more shiny than LE textures.
/// To fix this, alpha has to be made opaque
auto can_be_optimized_landscape(const Texture &file, const Settings &sets) -> bool
{
    const auto &tex         = file.get();
    const auto path         = canonize_path(file.get_load_path());
    const bool is_landscape = btu::common::contains(sets.landscape_textures, path);
    if (!is_landscape)
        return false;

    return !tex.IsAlphaAllOpaque();
}

auto compute_optimization_steps(const Texture &file, const Settings &sets) noexcept -> OptimizationSteps
{
    const auto &tex  = file.get();
    const auto &info = tex.GetMetadata();

    auto res = OptimizationSteps{};

    if (sets.compress && !DirectX::IsCompressed(info.format))
        res.format = sets.output_format;

    if (sets.use_format_whitelist && !btu::common::contains(sets.allowed_formats, info.format))
        res.format = sets.output_format;

    const auto dim                            = Dimension{info.width, info.height};
    const std::optional<Dimension> target_dim = std::visit(
        btu::common::overload{
            [](std::monostate) -> std::optional<Dimension> { return {}; },
            [&](util::ResizeRatio r) { return std::optional(util::compute_resize_dimension(dim, r)); },
            [&](Dimension target) { return std::optional(util::compute_resize_dimension(dim, target)); },
        },
        sets.resize);

    if (target_dim.has_value() && dim != target_dim.value())
        res.resize = target_dim.value();

    if (sets.game == btu::common::Game::SSE)
        if (can_be_optimized_landscape(file, sets))
            res.add_opaque_alpha = true;

    if (sets.mipmaps && optimal_mip_count(file.get_dimension()) != info.mipLevels)
        res.mipmaps = true;

    return res;
}

auto Settings::get(common::Game game) noexcept -> const Settings &
{
    // FIXME
    static const auto default_sets = Settings{.game = game};
    return default_sets;
}

} // namespace btu::tex
