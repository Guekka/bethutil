/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/optimize.hpp"

#include "./utils.hpp"
#include "btu/tex/compression_device.hpp"
#include "btu/tex/texture.hpp"

#include <DirectXTex.h>
#include <btu/tex/optimize.hpp>

#include <iostream>

auto operator<<(std::ostream &os, const btu::tex::OptimizationSteps &s) -> std::ostream &
{
    const auto dim = s.resize.value_or(btu::tex::Dimension{});
    return os << "add_trans_alpha: " << s.add_transparent_alpha << ";format: "
              << btu::common::as_ascii(btu::tex::to_string(s.format.value_or(DXGI_FORMAT_UNKNOWN))) << ""
              << ";mips: " << s.mipmaps << "; resize x:" << dim.w << " y:" << dim.h;
}

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
    sets.landscape_textures   = {u8"landscape/file.dds"};
    return sets;
};

const auto generate_tex2 = [] {
    auto tex = DirectX::ScratchImage{};
    tex.Initialize(generate_info2());

    // We create a texture with opaque alpha
    constexpr auto transform = [](DirectX::XMVECTOR *out_pixels,
                                  const DirectX::XMVECTOR *,
                                  const size_t width,
                                  [[maybe_unused]] size_t) {
        const auto color = DirectX::XMVectorSet(1, 1, 1, 1);
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
    REQUIRE(timage.IsAlphaAllOpaque());

    auto file = btu::tex::Texture{};
    file.set(std::move(timage));
    file.set_load_path(u8"textures/landscape/file.dds");
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
        CHECK(res.add_transparent_alpha == false);
        CHECK(res.mipmaps == true);
        CHECK(res.format == DXGI_FORMAT_BC7_UNORM);
    }
    SECTION("tex2")
    {
        auto tex = generate_tex2();
        auto res = compute_optimization_steps(tex, generate_sets2());
        CHECK(res.resize == std::nullopt);
        CHECK(res.add_transparent_alpha == true);
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
        auto dev   = btu::tex::CompressionDevice::make(0).value();
        auto res   = optimize(std::move(tex), steps, dev.get_device());
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
        auto dev   = btu::tex::CompressionDevice::make(0).value();
        auto res   = optimize(std::move(tex), steps, dev.get_device());
        REQUIRE(res.has_value());

        const auto res_info = res->get().GetMetadata();

        CHECK(res_info.width == 1024);
        CHECK(res_info.height == 1024);
        CHECK(res_info.depth == 1);
        CHECK(res_info.arraySize == 1);
        CHECK(res_info.mipLevels == 1);
        CHECK(res_info.format == DXGI_FORMAT_R8G8B8A8_UNORM);
        CHECK(res_info.dimension == DirectX::TEX_DIMENSION_TEXTURE2D);

        CHECK(!res->get().IsAlphaAllOpaque());
    }
    SECTION("expected_dir")
    {
        auto sets   = generate_sets1();
        sets.resize = btu::tex::Dimension{128, 128};
        test_expected_dir(u8"optimize", [&](auto &&f) {
            thread_local auto dev                   = btu::tex::CompressionDevice::make(0).value();
            const btu::tex::OptimizationSteps steps = btu::tex::compute_optimization_steps(f, sets);
            return btu::tex::optimize(std::move(f), steps, dev.get_device());
        });
    }
}
