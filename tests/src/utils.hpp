/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/tex/dimension.hpp"
#include "btu/tex/texture.hpp"

#include <catch2/catch.hpp>

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
    static auto convert(const btu::tex::ScratchImage &) -> std::string { return "scratch_image"; }
};
} // namespace Catch

inline auto load_tex(const std::filesystem::path &path) -> btu::tex::Texture
{
    btu::tex::Texture tex;
    auto res = tex.load_file(path);
    INFO(path);
    REQUIRE(res.has_value());
    return tex;
}

template<typename Func>
auto test_expected(const std::filesystem::path &root,
                   const std::filesystem::path &filename,
                   Func f,
                   bool approve = true)
{
    auto in                    = load_tex(root / "in" / filename);
    const btu::tex::Result out = f(std::move(in));
    if (!out.has_value())
    {
        UNSCOPED_INFO(out.error().ec.message());
        UNSCOPED_INFO(out.error().loc);
    }
    REQUIRE(out.has_value());

    const auto expected_path = root / "expected" / filename;
    if (!std::filesystem::exists(expected_path) && approve)
    {
        const auto res = out.value().save_file(expected_path);
        CHECK(res.has_value());
        FAIL("Expected file not found:" + expected_path.string());
    }
    else
    {
        const auto expected = load_tex(expected_path);
        CHECK(out->get() == expected.get());
    }
}

template<typename Func>
auto test_expected_dir(const std::filesystem::path &root, const Func &f) -> void
{
    for (const auto &file : std::filesystem::directory_iterator(root / "in"))
        if (file.is_regular_file())
            test_expected(root, file.path().filename(), f);
}
