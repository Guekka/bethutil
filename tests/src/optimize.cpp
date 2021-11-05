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

const auto generate_tex1 = [] {
    const DirectX::TexMetadata info = generate_info1();

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
    sets.output_format        = DXGI_FORMAT_BC7_UNORM;
    sets.resize               = btu::tex::util::ResizeRatio{.ratio = 7, .min = {256, 256}};
    return sets;
};

using btu::tex::compute_optimization_steps, btu::tex::optimize;

TEST_CASE("compute_optimization_steps")
{
    auto tex = generate_tex1();
    auto res = compute_optimization_steps(tex, generate_sets1());
    CHECK(res.resize == btu::tex::Dimension{256, 256});
    CHECK(res.add_opaque_alpha == false);
    CHECK(res.mipmaps == true);
    CHECK(res.format == DXGI_FORMAT_BC7_UNORM);
}

TEST_CASE("optimize")
{
    SECTION("tex1")
    {
        auto tex   = generate_tex1();
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
}
