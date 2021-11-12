/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/optimize.hpp"

#include "btu/tex/compression_device.hpp"
#include "btu/tex/detail/functional.hpp"
#include "btu/tex/functions.hpp"

#include <DirectXTex.h>
#include <btu/common/algorithms.hpp>
#include <btu/common/functional.hpp>

namespace btu::tex {
auto optimize(Texture &&file, OptimizationSteps sets, ID3D11Device *dev) noexcept -> Result
{
    using btu::common::bind_back;

    const auto compressed = DirectX::IsCompressed(file.get().GetMetadata().format);
    auto res              = Result{std::move(file)};

    if (compressed)
        res = std::move(res).and_then(decompress);
    if (sets.resize)
        res = std::move(res).and_then(bind_back(resize, sets.resize.value()));
    if (sets.add_transparent_alpha)
        res = std::move(res).and_then(make_transparent_alpha);
    if (sets.mipmaps)
        res = std::move(res).and_then(generate_mipmaps);
    if (sets.format && res && res.value().get().GetMetadata().format != sets.format)
        res = std::move(res).and_then(bind_back(convert, sets.format.value(), dev));

    return res;
}

/// SSE landscape textures uses alpha channel as specularity
/// Textures with opaque alpha are thus rendered shiny
/// To fix this, alpha has to be made transparent
auto can_be_optimized_landscape(const Texture &file, const Settings &sets) -> bool
{
    const auto &tex         = file.get();
    const auto path         = canonize_path(file.get_load_path());
    const bool is_landscape = btu::common::contains(sets.landscape_textures, path);
    if (!is_landscape)
        return false;

    return tex.IsAlphaAllOpaque();
}

[[nodiscard]] auto is_bad_cubemap(const TexMetadata &info) noexcept -> bool
{
    const bool isCubemap    = info.IsCubemap();
    const bool uncompressed = !DirectX::IsCompressed(info.format);

    const bool opaqueAlpha = info.GetAlphaMode() == DirectX::TEX_ALPHA_MODE::TEX_ALPHA_MODE_OPAQUE;
    const bool noAlpha     = !DirectX::HasAlpha(info.format);
    const bool badAlpha    = opaqueAlpha || noAlpha;

    return isCubemap && uncompressed && badAlpha;
}

[[nodiscard]] auto can_be_compressed(const TexMetadata &info) noexcept -> bool
{
    const bool too_small = info.width < 4 || info.height < 4;
    const bool pow2      = util::is_pow2(info.width) && util::is_pow2(info.height);

    return !too_small && pow2;
}

[[nodiscard]] auto compute_output_format(const Texture &file, const Settings &sets) noexcept
    -> std::optional<DXGI_FORMAT>
{
    const auto &info = file.get().GetMetadata();

    const bool forbidden_format = sets.use_format_whitelist
                                  && !btu::common::contains(sets.allowed_formats, info.format);

    const bool is_bad_cube = is_bad_cubemap(info);

    const bool bad = forbidden_format || is_bad_cube;
    if (!bad && !sets.compress) // All good, no need to change anything
        return std::nullopt;

    if (!sets.compress && bad) // No need to compress but texture is not good
        return guess_best_format(file, sets.output_format, true);

    return guess_best_format(file, sets.output_format, !can_be_compressed(info));
}

auto compute_optimization_steps(const Texture &file, const Settings &sets) noexcept -> OptimizationSteps
{
    const auto &tex  = file.get();
    const auto &info = tex.GetMetadata();

    auto res = OptimizationSteps{};

    res.format = compute_output_format(file, sets);

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
            res.add_transparent_alpha = true;

    const bool opt_mip = optimal_mip_count(file.get_dimension()) == info.mipLevels;
    if (sets.mipmaps && (!opt_mip || res.resize)) // resize removes mips
        res.mipmaps = true;

    return res;
}

auto Settings::get(common::Game game) noexcept -> const Settings &
{
    // FIXME
    static auto tes3_sets = [&] {
        return Settings{.game          = common::Game::TES3,
                        .output_format = {.uncompressed               = DXGI_FORMAT_R8G8B8A8_UNORM,
                                          .uncompressed_without_alpha = DXGI_FORMAT_R8G8B8A8_UNORM,
                                          .compressed                 = DXGI_FORMAT_BC3_UNORM,
                                          .compressed_without_alpha   = DXGI_FORMAT_BC1_UNORM}};
    }();

    switch (game)
    {
        case btu::common::Game::TES3: return tes3_sets;
        case btu::common::Game::TES4:
        {
            static auto tes4_sets = [&] {
                auto sets = tes3_sets;
                sets.game = btu::common::Game::TES4;
                return sets;
            }();
            return tes4_sets;
        }
        case btu::common::Game::FNV:
        {
            static auto fnv_sets = [&] {
                auto sets = tes3_sets;
                sets.game = btu::common::Game::FNV;
                return sets;
            }();
            return fnv_sets;
        }
        case btu::common::Game::SLE:
        {
            static auto sle_sets = [&] {
                auto sets = tes3_sets;
                sets.game = btu::common::Game::SLE;
                return sets;
            }();
            return sle_sets;
        }
        case btu::common::Game::SSE:
        {
            static auto sse_sets = [&] {
                auto sets                     = tes3_sets;
                sets.output_format.compressed = DXGI_FORMAT_BC7_UNORM;
                sets.game                     = btu::common::Game::SSE;
                return sets;
            }();
            return sse_sets;
        }
        case btu::common::Game::FO4:
        {
            static auto fo4_sets = [&] {
                auto sets = Settings::get(common::Game::SSE);
                sets.game = btu::common::Game::FO4;
                return sets;
            }();
            return fo4_sets;
        }
        case btu::common::Game::Custom: return tes3_sets;
    }
    return tes3_sets;
}

} // namespace btu::tex
