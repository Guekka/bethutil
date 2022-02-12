/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "../utils.hpp"
#include "btu/tex/dimension.hpp"
#include "btu/tex/texture.hpp"

#include <btu/common/string.hpp>
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

template<>
struct StringMaker<std::u8string_view>
{
    static auto convert(const std::u8string &s) -> std::string { return btu::common::as_ascii_string(s); }
};

template<>
struct StringMaker<std::u8string>
{
    static auto convert(const std::u8string &s) -> std::string
    {
        return StringMaker<std::u8string_view>::convert(s);
    }
};
} // namespace Catch

inline auto load_tex(const Path &path) -> btu::tex::Texture
{
    btu::tex::Texture tex;
    auto res = tex.load_file(path);
    INFO(path);
    REQUIRE(res.has_value());
    return tex;
}

template<typename Func>
auto test_expected(const Path &root,
                   const Path &filename,
                   Func f,
                   bool approve = true)
{
    auto in                    = load_tex(root / "in" / filename);
    const btu::tex::Result out = f(std::move(in));

    REQUIRE(out.has_value());

    const auto expected_path = root / "expected" / filename;
    if (!fs::exists(expected_path) && approve)
    {
        fs::create_directories(expected_path.parent_path());
        const auto res = out.value().save_file(expected_path);
        CHECK(res.has_value());
        FAIL_CHECK("Expected file not found:" + expected_path.string());
    }
    else
    {
        const auto expected = load_tex(expected_path);
        CHECK(out->get() == expected.get());
    }
}

template<typename Func>
auto test_expected_dir(const Path &root, const Func &f) -> void
{
    const auto in_dir = root / "in";
    for (const auto &file : fs::recursive_directory_iterator(in_dir))
        if (file.is_regular_file())
            test_expected(root, file.path().lexically_relative(in_dir), f);
}
