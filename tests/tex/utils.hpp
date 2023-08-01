/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "../utils.hpp"
#include "btu/tex/dimension.hpp"
#include "btu/tex/texture.hpp"

#include <btu/common/string.hpp>
#include <catch.hpp>
#include <flux.hpp>

namespace Catch {
template<>
struct StringMaker<btu::tex::Dimension>
{
    static auto convert(const btu::tex::Dimension &value) -> std::string
    {
        auto ts = [](auto &&a) { return StringMaker<decltype(a)>::convert(a); };
        return "{" + ts(value.w) + ", " + ts(value.h) + "}";
    }
};
template<>
struct StringMaker<btu::tex::ScratchImage>
{
    static auto convert(const btu::tex::ScratchImage & /*unused*/) -> std::string { return "scratch_image"; }
};

} // namespace Catch

inline auto load_tex(const Path &path) -> btu::tex::Texture
{
    auto res = btu::tex::load(path);
    INFO(path);
    REQUIRE(res.has_value());
    return std::move(res).value();
}

/// Different encoders produce different results, so we need to compare them by hand
/// This function compares two textures using MSE and returns true if they are almost equal
inline auto compute_mse(const btu::tex::Texture &lhs, const btu::tex::Texture &rhs) -> float
{
    REQUIRE(lhs.get().GetMetadata() == rhs.get().GetMetadata());
    const size_t img_count = lhs.get().GetImageCount();

    float total_mse = 0; //  average of MSEs for each subimage
    for (size_t i = 0; i < img_count; ++i)
    {
        const auto &lhs_sub = lhs.get_images()[i];
        const auto &rhs_sub = rhs.get_images()[i];

        float mse     = 0;
        const auto hr = DirectX::ComputeMSE(lhs_sub, rhs_sub, mse, nullptr);
        REQUIRE(SUCCEEDED(hr));

        total_mse += mse;
    }
    total_mse /= static_cast<float>(img_count);
    return total_mse;
}

enum class Approve
{
    Yes,
    No,
};

template<typename Func>
auto test_expected(const Path &root, const Path &filename, Func f, Approve approve = Approve::No)
{
    auto in                    = load_tex(root / "in" / filename);
    const btu::tex::Result out = f(std::move(in));

    INFO("Processing: " << filename);
    if (!out)
    {
        FAIL_CHECK("Error: " << out.error());
    }

    const auto expected_path = root / "expected" / filename;
    if (!btu::fs::exists(expected_path) && approve == Approve::Yes)
    {
        btu::fs::create_directories(expected_path.parent_path());
        const auto res = btu::tex::save(out.value(), expected_path);
        CHECK(res.has_value());
        FAIL_CHECK("Expected file not found:" + expected_path.string());
    }
    else
    {
        const auto expected = load_tex(expected_path);
        INFO("Expected path: " << expected_path);

        constexpr float k_max_mse = 0.002F; // empirically determined
        const auto mse            = compute_mse(out.value(), expected);

        INFO("MSE: " << mse);
        CHECK(mse <= k_max_mse);
    }
}

template<typename Func>
void test_expected_dir(const Path &root, const Func &f)
{
    const auto in_dir = root / "in";
    for (const auto &file : btu::fs::recursive_directory_iterator(in_dir))
        if (file.is_regular_file())
            test_expected(root, file.path().lexically_relative(in_dir), f);
}
