/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/string.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <iostream>
#include <numeric>

namespace bc = btu::common;

namespace Catch {
template<>
struct StringMaker<std::u8string_view>
{
    static auto convert(const std::u8string_view &v) -> std::string
    {
        using bc::UTF8Iterator, bc::U8Unit;
        return std::string(bc::as_ascii(v)) + " ("
               + StringMaker<std::vector<U8Unit>>::convert(std::vector(UTF8Iterator(v), UTF8Iterator::end(v)))
               + ")";
    }
};

template<>
struct StringMaker<std::u8string>
{
    static auto convert(const std::u8string &v) -> std::string
    {
        return StringMaker<std::u8string_view>::convert(v);
    }
};

} // namespace Catch

TEST_CASE("UTF8 Iterator", "[string]")
{
    using bc::UTF8Iterator;
    SECTION("Random data")
    {
        const auto data = std::to_array<std::u8string>({
            u8R"(F)",  u8R"(񗑴)", u8R"(N)",    u8R"(:)",    u8R"(f)", u8R"(9)",   u8R"(򅁱)", u8R"(Ȁ)",
            u8R"(㈗)", u8R"({)",    u8R"(򦄃)", u8R"(g)",    u8R"()", u8R"(Μ)",   u8R"(\)",    u8R"(Ĺ)",
            u8R"(i)",  u8R"(ӆ)",    u8R"(ᔔ)",    u8R"(򊒏)", u8R"(ܨ)", u8R"(ꬵ)", u8R"(ρ)",    u8R"(Ħ)",
            u8R"(Ҥ)",  u8R"(ģ)",    u8R"(𦖋)",   u8R"(㱏)",   u8R"(ሐ)", u8R"(4)",   u8R"(=)",
        });

        const auto string = std::reduce(data.begin(), data.end(), std::u8string{});
        std::vector<bc::U8Unit> codepoints{};
        std::transform(data.begin(), data.end(), std::back_inserter(codepoints), [](auto &&str) {
            return bc::first_codepoint(str);
        });

        CHECK(std::equal(UTF8Iterator(string), UTF8Iterator::end(string), codepoints.cbegin()));
    }
}

TEST_CASE("as_utf8 / as_ascii", "[string]")
{
    constexpr auto *orig = u8R"(🮕🖜🞭📎🸘🴆🄧🂟🂰🖷🴚🎣👒🹓🱸🈪🗐🌦🋡)";
    CHECK(bc::as_utf8(bc::as_ascii(orig)) == orig);
}

TEST_CASE("str_compare", "[string]")
{
    using bc::str_compare;

    SECTION("Basic ASCII")
    {
        CHECK_FALSE(str_compare(u8"A", u8"a"));
        CHECK(str_compare(u8"A", u8"a", false));

        CHECK_FALSE(str_compare(u8"somepath/c/x/d!", u8"somepath/C/X/D!"));
        CHECK(str_compare(u8"somepath/c/x/d!", u8"somepath/C/X/D!", false));
    }
}

TEST_CASE("str_find", "[string]")
{
    using bc::str_find;

    CHECK(str_find(u8"abcdÀ👒<f¹øì►", u8"à👒") == std::string::npos);
    CHECK(str_find(u8"abcdÀ👒<f¹øì►", u8"À👒") == 4);
    CHECK(str_find(u8"abcdÀ👒<f¹øì►", u8"à👒", false) == 4);
}

TEST_CASE("str_contain", "[string]")
{
    using bc::str_contain;

    CHECK_FALSE(str_contain(u8"abcdÀ👒<f¹øì►", u8"à👒"));
    CHECK(str_contain(u8"abcdÀ👒<f¹øì►", u8"à👒", false));
}

TEST_CASE("to_lower", "[string]")
{
    using namespace std::literals;

    const auto data = std::to_array<std::pair<std::u8string, std::u8string>>({
        {u8"ABCDEFGHIJKLMNOPQRSTUVXYZ", u8"abcdefghijklmnopqrstuvxyz"},
        {u8"abcdefghijklmnopqrstuvxyz", u8"abcdefghijklmnopqrstuvxyz"},
        {u8"&\"'(-_),;:!", u8"&\"'(-_),;:!"},
        {u8"À", u8"à"},
        {u8"ÀÉ", u8"àé"},
        {u8"ß", u8"ß"},
    });

    for (const auto &[upper, lower] : data)
        CHECK(lower == bc::to_lower(upper));
}

TEST_CASE("first_codepoint", "[string]")
{
    using btu::common::first_codepoint;

    const auto data = std::to_array<std::pair<btu::common::U8Unit, std::u8string>>({
        {0, u8R"()"},
        {1222, u8R"(ӆ)"},
        {5396, u8R"(ᔔ)"},
        {566415, u8R"(򊒏)"},
        {1832, u8R"(ܨ)"},
        {291, u8R"(ģ)"},
    });

    for (const auto &[code, str] : data)
    {
        CHECK(first_codepoint(str) == code);
    }
}

TEST_CASE("concat_codepoint", "[string]")
{
    using btu::common::concat_codepoint, btu::common::UTF8Iterator;

    std::u8string str;
    std::vector<btu::common::U8Unit> units;
    auto add_codepoint = [&](auto cp) {
        concat_codepoint(str, cp);
        units.emplace_back(cp);
        std::vector str_units(UTF8Iterator(str), UTF8Iterator::end(str));
        CHECK(units == str_units);
    };

    for (auto &&cp : {291, 1222, 566415, 1832, 5396})
        add_codepoint(cp);
}
