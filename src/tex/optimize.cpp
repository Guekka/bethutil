/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/optimize.hpp"

#include "btu/tex/compression_device.hpp"
#include "btu/tex/functions.hpp"

#include <btu/common/algorithms.hpp>
#include <btu/common/metaprogramming.hpp>
#include <btu/tex/dxtex.hpp>

#include <regex>

namespace btu::tex {

auto optimize(Texture &&file, OptimizationSteps sets, CompressionDevice &dev) noexcept -> Result
{
    const auto must_decompress = (sets.resize || sets.add_transparent_alpha)
                                 && DirectX::IsCompressed(file.get().GetMetadata().format);
    auto res = Result{std::move(file)};

    // We must only decompress if we want to resize or transform the image.
    if (must_decompress)
        res = std::move(res).and_then(decompress);
    if (sets.resize)
        res = std::move(res).and_then(
            [&](Texture &&tex) { return resize(std::move(tex), sets.resize.value()); });
    if (sets.add_transparent_alpha)
        res = std::move(res).and_then(make_transparent_alpha);
    if (sets.mipmaps)
        res = std::move(res).and_then(generate_mipmaps);

    // We have uncompressed the texture. If it was compressed, it's best to convert it to a better format
    const auto cur_format_is_same_as_best = res && res->get().GetMetadata().format == sets.best_format;

    if ((sets.convert || must_decompress) && !cur_format_is_same_as_best)
    {
        auto out = sets.best_format;
        res      = std::move(res)
                  // // safety check: make sure we don't remove the alpha
                  .and_then([&](Texture &&tex) -> Result {
                      if (!DirectX::HasAlpha(out) && !tex.get().IsAlphaAllOpaque())
                          return tl::make_unexpected(Error(TextureErr::BadInput));
                      return std::move(tex);
                  })
                  .and_then([&](Texture &&tex) { return convert(std::move(tex), out, dev); });
    }

    return res;
}

/// SSE landscape textures uses alpha channel as specularity
/// Textures with opaque alpha are thus rendered shiny
/// To fix this, alpha has to be made transparent
auto transparent_alpha_required(const Texture &file, const Settings &sets) -> bool
{
    const auto &tex         = file.get();
    const auto path         = canonize_path(file.get_load_path());
    const bool is_landscape = common::contains(sets.landscape_textures, path);
    if (!is_landscape)
        return false;

    return tex.IsAlphaAllOpaque();
}

[[nodiscard]] auto is_bad_cubemap(const TexMetadata &info) noexcept -> bool
{
    const bool is_cubemap   = info.IsCubemap();
    const bool uncompressed = !DirectX::IsCompressed(info.format);

    const bool opaque_alpha = info.GetAlphaMode() == DirectX::TEX_ALPHA_MODE::TEX_ALPHA_MODE_OPAQUE;
    const bool no_alpha     = !DirectX::HasAlpha(info.format);
    const bool bad_alpha    = opaque_alpha || no_alpha;

    return is_cubemap && uncompressed && bad_alpha;
}

[[nodiscard]] auto has_opaque_alpha(const ScratchImage &tex) noexcept -> bool
{
    using enum DirectX::TEX_ALPHA_MODE;

    const auto has_alpha  = DirectX::HasAlpha(tex.GetMetadata().format);
    const auto alpha_mode = tex.GetMetadata().GetAlphaMode();
    return !has_alpha || alpha_mode == TEX_ALPHA_MODE_OPAQUE || tex.IsAlphaAllOpaque();
};

[[nodiscard]] auto can_be_compressed(const TexMetadata &info) noexcept -> bool
{
    const bool too_small = info.width < 4 || info.height < 4;
    const bool pow2      = util::is_pow2(info.width) && util::is_pow2(info.height);

    return !too_small && pow2;
}

[[nodiscard]] auto is_tga(const Texture &file) noexcept -> bool
{
    const auto ext = file.get_load_path().extension();
    return str_compare(ext.u8string(), u8".tga", common::CaseSensitive::No);
}

[[nodiscard]] auto conversion_required(const Texture &file, const Settings &sets) noexcept -> bool
{
    const auto &info = file.get().GetMetadata();

    const bool forbidden_format = sets.use_format_whitelist
                                  && !common::contains(sets.allowed_formats, info.format);

    const bool is_bad_cube = is_bad_cubemap(info);
    const bool bad         = forbidden_format || is_bad_cube;

    return bad || is_tga(file);
}

[[nodiscard]] auto best_output_format(const Texture &file,
                                      const Settings &sets,
                                      bool force_alpha) noexcept -> DXGI_FORMAT
{
    const auto &tex  = file.get();
    const auto &info = tex.GetMetadata();

    return guess_best_format(info.format,
                             sets.output_format,
                             GuessBestFormatArgs{.opaque_alpha     = has_opaque_alpha(tex),
                                                 .allow_compressed = sets.compress && can_be_compressed(info),
                                                 .force_alpha      = force_alpha});
}

auto compute_optimization_steps(const Texture &file, const Settings &sets) noexcept -> OptimizationSteps
{
    const auto &tex  = file.get();
    const auto &info = tex.GetMetadata();

    auto res = OptimizationSteps{};

    // Check if file is ignored.
    auto path = canonize_path(file.get_load_path());
    for (auto &pattern : sets.ignored_files)
    {
        auto regex = std::regex{btu::common::as_ascii_string(pattern),
                                std::regex::optimize | std::regex::icase};

        if (std::regex_match(common::as_ascii_string(path), regex))
            return res;
    }

    // Check if conversion is a must.
    res.convert = conversion_required(file, sets);

    // Do not compress the image if already compressed.
    if (sets.compress && !DirectX::IsCompressed(info.format))
        res.convert = true;

    const auto dim = Dimension{info.width, info.height};
    const std::optional<Dimension> target_dim
        = std::visit(common::Overload{
                         [](std::monostate) -> std::optional<Dimension> { return {}; },
                         [&](util::ResizeRatio r) { return std::optional(compute_resize_dimension(dim, r)); },
                         [&](Dimension target) {
                             return std::optional(util::compute_resize_dimension(dim, target));
                         },
                     },
                     sets.resize);

    if (target_dim.has_value() && dim != target_dim.value())
        res.resize = target_dim.value();

    if (sets.game == Game::SSE)
        if (transparent_alpha_required(file, sets))
            res.add_transparent_alpha = true;

    const bool opt_mip = optimal_mip_count(file.get_dimension()) == info.mipLevels;
    if ((sets.mipmaps && !opt_mip)
        || (info.mipLevels > 1 && res.resize)) // resize removes mips if there are any, regenerate them
        res.mipmaps = true;

    // I prefer to keep steps independent, but this one has to depend on add_transparent_alpha. If we add an alpha, the output format must have alpha
    res.best_format = best_output_format(file, sets, res.add_transparent_alpha);

    // Ensure result is saved without alpha in cases where it could be problematic (i.e. resizing).
    if (res.resize && res.best_format == sets.output_format.uncompressed_without_alpha)
        res.convert = true;

    return res;
}

// NOLINTNEXTLINE(misc-no-recursion)
auto Settings::get(Game game) noexcept -> const Settings &
{
    static auto tes3_sets = [&] {
        return Settings{
            .game                 = Game::TES3,
            .compress             = false,
            .resize               = {},
            .mipmaps              = true,
            .use_format_whitelist = true,
            .allowed_formats      = {
                DXGI_FORMAT_BC3_UNORM,
                DXGI_FORMAT_BC1_UNORM,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_FORMAT_B8G8R8A8_UNORM,
                DXGI_FORMAT_B8G8R8X8_UNORM,
            }, // Older titles only support DXT1-DXT5 compression.
            .output_format        = {.uncompressed               = DXGI_FORMAT_B8G8R8A8_UNORM,
                                     .uncompressed_without_alpha = DXGI_FORMAT_B8G8R8X8_UNORM,
                                     .compressed                 = DXGI_FORMAT_BC3_UNORM,
                                     .compressed_without_alpha   = DXGI_FORMAT_BC1_UNORM},
            .landscape_textures   = {}, // Unknown
            .ignored_files        = {}, // Ignore nothing by default.
        };
    }();

    switch (game)
    {
        case Game::TES3: return tes3_sets;
        case Game::TES4:
        {
            static auto tes4_sets = [&] {
                auto sets = tes3_sets;
                sets.game = Game::TES4;
                return sets;
            }();
            return tes4_sets;
        }
        case Game::FNV:
        {
            static auto fnv_sets = [&] {
                auto sets          = tes3_sets;
                sets.game          = Game::FNV;
                sets.ignored_files = {u8"interface/.*", u8".*lod(_[nsdgpe])?\\.dds"};
                return sets;
            }();
            return fnv_sets;
        }
        case Game::SLE:
        {
            static auto sle_sets = [&] {
                auto sets = tes3_sets;
                sets.game = Game::SLE;
                return sets;
            }();
            return sle_sets;
        }
        case Game::SSE:
        {
            static auto sse_sets = [&] {
                auto sets            = tes3_sets;
                sets.compress        = true;
                sets.allowed_formats = {
                    DXGI_FORMAT_BC7_UNORM,
                    DXGI_FORMAT_BC5_UNORM,
                    DXGI_FORMAT_BC3_UNORM,
                    DXGI_FORMAT_BC1_UNORM,
                    DXGI_FORMAT_R8G8B8A8_UNORM,
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    DXGI_FORMAT_B8G8R8X8_UNORM,
                };
                sets.output_format.compressed               = DXGI_FORMAT_BC7_UNORM;
                sets.output_format.compressed_without_alpha = DXGI_FORMAT_BC7_UNORM;
                sets.game                                   = Game::SSE;
                return sets;
            }();
            return sse_sets;
        }
        case Game::FO4:
        {
            // NOLINTNEXTLINE(misc-no-recursion)
            static auto fo4_sets = [&] {
                auto sets = get(Game::SSE);
                sets.game = Game::FO4;
                return sets;
            }();
            return fo4_sets;
        }
        case Game::Starfield:
        {
            // NOLINTNEXTLINE(misc-no-recursion)
            static auto starfield_sets = [&] {
                auto sets = get(Game::SSE);
                sets.game = Game::Starfield;
                return sets;
            }();
            return starfield_sets;
        }
        case Game::Custom: return tes3_sets;
    }
    return tes3_sets;
}

} // namespace btu::tex
