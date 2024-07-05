/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "./utils.hpp"

#include <btu/tex/dxtex.hpp>
#include <btu/tex/optimize.hpp>
#include <btu/tex/texture.hpp>

auto operator<<(std::ostream &os, const btu::tex::OptimizationSteps &s) -> std::ostream &
{
    const auto dim = s.resize.value_or(btu::tex::Dimension{});
    return os << "add_trans_alpha: " << s.add_transparent_alpha
              << ";format: " << btu::common::as_ascii(btu::tex::to_string(s.best_format)) << ""
              << ";mips: " << s.mipmaps << "; resize x:" << dim.w << " y:" << dim.h;
}

constexpr auto bc5_512_no_mips_meta = [] {
    return DirectX::TexMetadata{
        .width      = 512,
        .height     = 512,
        .depth      = 1,
        .arraySize  = 1,
        .mipLevels  = 1,
        .miscFlags  = 0,
        .miscFlags2 = 0,
        .format     = DXGI_FORMAT_BC5_UNORM,
        .dimension  = DirectX::TEX_DIMENSION_TEXTURE2D,
    };
}();

auto generate_tex(const DirectX::TexMetadata &info) -> btu::tex::Texture
{
    auto image    = DirectX::ScratchImage{};
    const auto hr = image.Initialize(info);
    REQUIRE(SUCCEEDED(hr));
    auto tex = btu::tex::Texture{};
    tex.set(std::move(image));

    return tex;
}

const auto compress_whitelist_mips_resize_sets = []() noexcept {
    auto sets                 = btu::tex::Settings::get(btu::Game::SSE);
    sets.use_format_whitelist = true;
    sets.output_format        = btu::tex::BestFormatFor{
               .uncompressed               = DXGI_FORMAT_R8G8B8A8_UNORM,
               .uncompressed_without_alpha = DXGI_FORMAT_P8,
               .compressed                 = DXGI_FORMAT_BC1_UNORM,
               .compressed_without_alpha   = DXGI_FORMAT_BC5_UNORM,
    };
    sets.allowed_formats = {DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};
    sets.compress        = true;
    sets.mipmaps         = true;
    sets.resize          = btu::tex::util::ResizeRatio{.ratio = 7, .min = {128, 128}};
    return sets;
}();

constexpr auto r8g8b8a8_512_no_mips_meta = [] {
    auto info   = bc5_512_no_mips_meta;
    info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    return info;
}();

constexpr auto bc7_512_no_mips_meta = [] {
    auto info   = bc5_512_no_mips_meta;
    info.format = DXGI_FORMAT_BC7_UNORM;
    return info;
}();

const auto landscape_sets = [] {
    auto sets                 = compress_whitelist_mips_resize_sets;
    sets.use_format_whitelist = false;
    sets.compress             = false;
    sets.mipmaps              = false;
    sets.resize               = std::monostate{};
    sets.landscape_textures   = {u8"landscape/file.dds"};
    return sets;
}();

void add_opaque_alpha(DirectX::ScratchImage &tex)
{
    // We create a texture with opaque alpha
    constexpr auto transform = [](DirectX::XMVECTOR *out_pixels,
                                  const DirectX::XMVECTOR *,
                                  const size_t width,
                                  [[maybe_unused]] size_t) {
        const auto color = DirectX::XMVectorSet(1, 1, 1, 1);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::fill_n(out_pixels, width, color);
    };

    DirectX::ScratchImage timage;
    const auto hr = TransformImage(tex.GetImages(), tex.GetImageCount(), tex.GetMetadata(), transform, timage);
    REQUIRE(hr >= 0);
    REQUIRE(timage.IsAlphaAllOpaque());

    tex = std::move(timage);
}

[[nodiscard]] auto generate_landscape_texture(DirectX::TexMetadata meta) -> btu::tex::Texture
{
    auto tex = generate_tex(meta);
    // if format does not have alpha, it's already opaque
    if (DirectX::HasAlpha(meta.format))
        add_opaque_alpha(tex.get());
    tex.set_load_path(u8"textures/landscape/file.dds");
    return tex;
}

using btu::tex::compute_optimization_steps, btu::tex::optimize;

TEST_CASE("compute_optimization_steps", "[src]")
{
    SECTION("bc5_512_no_mips_meta")
    {
        auto tex = generate_tex(bc5_512_no_mips_meta);
        auto res = compute_optimization_steps(tex, compress_whitelist_mips_resize_sets);
        CHECK(res.resize == btu::tex::Dimension{128, 128});
        CHECK_FALSE(res.add_transparent_alpha);
        CHECK(res.mipmaps);
        CHECK(res.best_format == compress_whitelist_mips_resize_sets.output_format.compressed_without_alpha);
        CHECK(res.convert);
    }
    SECTION("landscape texture")
    {
        auto tex = generate_landscape_texture(r8g8b8a8_512_no_mips_meta);

        auto res = compute_optimization_steps(tex, landscape_sets);
        CHECK(res.resize == std::nullopt);
        CHECK(res.add_transparent_alpha);
        CHECK_FALSE(res.mipmaps);
        CHECK(res.best_format == landscape_sets.output_format.compressed);
        CHECK_FALSE(res.convert);
    }
    SECTION("do not compress to same format as current")
    {
        auto tex       = generate_tex(bc7_512_no_mips_meta);
        auto sets      = compress_whitelist_mips_resize_sets;
        sets.resize    = {};
        const auto res = compute_optimization_steps(tex, sets);
        CHECK(res.resize == std::nullopt);
        CHECK_FALSE(res.add_transparent_alpha);
        CHECK(res.mipmaps);
        CHECK(res.best_format == sets.output_format.compressed);
        CHECK_FALSE(res.convert);
    }
    SECTION("do not recompress if already compressed")
    {
        auto tex      = generate_tex(bc5_512_no_mips_meta);
        auto sets     = landscape_sets;
        sets.compress = true;

        const auto res = compute_optimization_steps(tex, sets);
        CHECK(res.resize == std::nullopt);
        CHECK_FALSE(res.add_transparent_alpha);
        CHECK_FALSE(res.mipmaps);
        CHECK(res.best_format == sets.output_format.compressed_without_alpha);
        CHECK_FALSE(res.convert);
    }
    SECTION("best_format has alpha if add_transparent_alpha, even on texture without alpha")
    {
        auto tex = generate_landscape_texture(bc5_512_no_mips_meta);

        const auto res = compute_optimization_steps(tex, landscape_sets);
        CHECK(res.resize == std::nullopt);
        CHECK(res.add_transparent_alpha);
        CHECK_FALSE(res.mipmaps);
        CHECK(res.best_format == landscape_sets.output_format.compressed);
        CHECK_FALSE(res.convert);
    }
}

TEST_CASE("tex_optimize", "[src]")
{
    SECTION("bc5_512_no_mips_meta")
    {
        auto tex   = generate_tex(bc5_512_no_mips_meta);
        auto steps = compute_optimization_steps(tex, compress_whitelist_mips_resize_sets);
        auto res   = optimize(std::move(tex), steps, compression_dev);
        REQUIRE(res.has_value());

        const auto res_info = res->get().GetMetadata();

        CHECK(res_info.width == 128);
        CHECK(res_info.height == 128);
        CHECK(res_info.depth == 1);
        CHECK(res_info.arraySize == 1);
        CHECK(res_info.mipLevels == 8);
        CHECK(res_info.format == compress_whitelist_mips_resize_sets.output_format.compressed_without_alpha);
        CHECK(res_info.dimension == DirectX::TEX_DIMENSION_TEXTURE2D);
    }
    SECTION("landscape texture")
    {
        auto tex   = generate_landscape_texture(r8g8b8a8_512_no_mips_meta);
        auto steps = compute_optimization_steps(tex, landscape_sets);
        auto res   = optimize(std::move(tex), steps, compression_dev);
        REQUIRE(res.has_value());

        const auto res_info = res->get().GetMetadata();

        CHECK(res_info.width == 512);
        CHECK(res_info.height == 512);
        CHECK(res_info.depth == 1);
        CHECK(res_info.arraySize == 1);
        CHECK(res_info.mipLevels == 1);
        CHECK(res_info.format == landscape_sets.output_format.uncompressed);
        CHECK(res_info.dimension == DirectX::TEX_DIMENSION_TEXTURE2D);

        CHECK(!res->get().IsAlphaAllOpaque());
    }
    SECTION("fails if output format would remove alpha")
    {
        auto tex          = generate_landscape_texture(bc5_512_no_mips_meta);
        auto steps        = compute_optimization_steps(tex, landscape_sets);
        steps.best_format = DXGI_FORMAT_BC5_UNORM; // no alpha
        const auto res    = optimize(std::move(tex), steps, compression_dev);
        CHECK_FALSE(res.has_value());
        CHECK(res.error() == btu::tex::TextureErr::BadInput);
    }
    SECTION("expected_dir")
    {
        auto sets = compress_whitelist_mips_resize_sets;
        // go back to more common settings
        sets.output_format = btu::tex::Settings::get(btu::Game::SSE).output_format;
        sets.resize        = btu::tex::Dimension{128, 128};
        test_expected_dir(u8"optimize", [&](auto &&f) {
            const btu::tex::OptimizationSteps steps = btu::tex::compute_optimization_steps(f, sets);
            return btu::tex::optimize(std::forward<decltype(f)>(f), steps, compression_dev);
        });
    }
}
