/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/optimize.hpp"

#include "btu/tex/texture.hpp"

#include <DirectXTex.h>
#include <btu/tex/optimize.hpp>
#include <catch.hpp>

const auto generate_info1 = [] {
    return DirectX::TexMetadata{
        .width     = 1024,
        .height    = 1024,
        .depth     = 1,
        .arraySize = 1,
        .mipLevels = 1,
        .format    = DXGI_FORMAT_BC5_UNORM,
        .dimension = DirectX::TEX_DIMENSION_TEXTURE2D,
    };
};

const auto generate_tex = [](const auto &info) {
    auto image = DirectX::ScratchImage{};
    image.Initialize(info);
    auto tex = btu::tex::Texture{};
    tex.set(std::move(image));

    return tex;
};

const auto generate_sets1 = [] {
    auto sets                 = btu::tex::Settings::get(btu::common::Game::SSE);
    sets.use_format_whitelist = true;
    sets.allowed_formats      = {DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};
    sets.compress             = true;
    sets.mipmaps              = true;
    sets.resize               = btu::tex::util::ResizeRatio{.ratio = 7, .min = {256, 256}};
    return sets;
};

const auto generate_info2 = [] {
    auto info   = generate_info1();
    info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    return info;
};

const auto generate_sets2 = [] {
    auto sets                 = btu::tex::Settings::get(btu::common::Game::SSE);
    sets.use_format_whitelist = false;
    sets.compress             = false;
    sets.mipmaps              = false;
    sets.resize               = std::monostate{};
    sets.landscape_textures   = {u8"textures/file.dds"};
    return sets;
};

const auto generate_tex2 = [] {
    auto tex = DirectX::ScratchImage{};
    tex.Initialize(generate_info2());

    // We create a texture with white alpha
    constexpr auto transform = [](DirectX::XMVECTOR *out_pixels,
                                  const DirectX::XMVECTOR *,
                                  const size_t width,
                                  [[maybe_unused]] size_t) {
        const auto color = DirectX::XMVectorSet(0.250, 0.25, .25, .5);
        const auto end   = out_pixels + width; // NOLINT
        std::fill(out_pixels, end, color);
    };

    DirectX::ScratchImage timage;
    const auto hr = DirectX::TransformImage(tex.GetImages(),
                                            tex.GetImageCount(),
                                            tex.GetMetadata(),
                                            transform,
                                            timage);

    REQUIRE(hr >= 0);
    REQUIRE_FALSE(timage.IsAlphaAllOpaque());

    auto file = btu::tex::Texture{};
    file.set(std::move(timage));
    file.set_load_path(u8"textures/file.dds");
    return file;
};

using btu::tex::compute_optimization_steps, btu::tex::optimize;

TEST_CASE("compute_optimization_steps")
{
    SECTION("tex1")
    {
        auto tex = generate_tex(generate_info1());
        auto res = compute_optimization_steps(tex, generate_sets1());
        CHECK(res.resize == btu::tex::Dimension{256, 256});
        CHECK(res.add_opaque_alpha == false);
        CHECK(res.mipmaps == true);
        CHECK(res.format == DXGI_FORMAT_BC7_UNORM);
    }
    SECTION("tex2")
    {
        auto tex = generate_tex2();
        auto res = compute_optimization_steps(tex, generate_sets2());
        CHECK(res.resize == std::nullopt);
        CHECK(res.add_opaque_alpha == true);
        CHECK(res.mipmaps == false);
        CHECK(res.format == std::nullopt);
    }
}

TEST_CASE("optimize")
{
    {
        auto tex   = generate_tex(generate_info1());
        auto sets  = generate_sets1();
        auto steps = compute_optimization_steps(tex, sets);
        auto res   = optimize(std::move(tex), steps);
        REQUIRE(res.has_value());

        const auto res_info = res->get().GetMetadata();

        CHECK(res_info.width == 256);
        CHECK(res_info.height == 256);
        CHECK(res_info.depth == 1);
        CHECK(res_info.arraySize == 1);
        CHECK(res_info.mipLevels == 9);
        CHECK(res_info.format == DXGI_FORMAT_BC7_UNORM);
        CHECK(res_info.dimension == DirectX::TEX_DIMENSION_TEXTURE2D);
    }
    SECTION("tex2")
    {
        auto tex   = generate_tex2();
        auto sets  = generate_sets2();
        auto steps = compute_optimization_steps(tex, sets);
        auto res   = optimize(std::move(tex), steps);
        REQUIRE(res.has_value());

        const auto res_info = res->get().GetMetadata();

        CHECK(res_info.width == 1024);
        CHECK(res_info.height == 1024);
        CHECK(res_info.depth == 1);
        CHECK(res_info.arraySize == 1);
        CHECK(res_info.mipLevels == 1);
        CHECK(res_info.format == DXGI_FORMAT_R8G8B8A8_UNORM);
        CHECK(res_info.dimension == DirectX::TEX_DIMENSION_TEXTURE2D);

        CHECK(res->get().IsAlphaAllOpaque());
    }
}
