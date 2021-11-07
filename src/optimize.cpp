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

namespace btu::tex {
auto optimize(Texture &&file, OptimizationSteps sets) noexcept -> Result
{
    auto dev              = CompressionDevice::make(0).value();
    const auto compressed = DirectX::IsCompressed(file.get().GetMetadata().format);
    auto res              = Result{std::move(file)};

    if (compressed)
        res = std::move(res).and_then(decompress);
    if (sets.resize)
        res = std::move(res).and_then(detail::bind_back(resize, sets.resize.value()));
    if (sets.add_opaque_alpha)
        res = std::move(res).and_then(make_opaque_alpha);
    if (sets.mipmaps)
        res = std::move(res).and_then(generate_mipmaps);
    if (sets.format)
        res = std::move(res).and_then(detail::bind_back(convert, sets.format.value(), std::ref(dev)));

    return res;
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

    const bool forbidden_format = sets.use_format_whitelist
                                  && !btu::common::contains(sets.allowed_formats, info.format);
    const bool compress = sets.compress && !DirectX::IsCompressed(info.format);
    const bool good_compress_size = info.width >= 4 && info.height >= 4;

    if (good_compress_size && !(forbidden_format || compress))
        res.format = guess_best_format(file, sets.output_format);

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
