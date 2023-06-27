/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/string.hpp"

#include <catch.hpp>

#include <array>
#include <numeric>

namespace bc = btu::common;

TEST_CASE("UTF8 Iterator", "[src]")
{
    using bc::UTF8Iterator;
    SECTION("Random data")
    {
        const auto data = std::to_array<std::u8string>({
            u8R"(F)",  u8R"(񗑴)", u8R"(N)",    u8R"(:)",    u8R"(f)",   u8R"(9)", u8R"(򅁱)", u8R"(Ȁ)",
            u8R"(㈗)", u8R"({)",    u8R"(򦄃)", u8R"(g)",    u8R"()", u8R"(Μ)", u8R"(\)",    u8R"(Ĺ)",
            u8R"(i)",  u8R"(ӆ)",    u8R"(ᔔ)",    u8R"(򊒏)", u8R"(ܨ)",   u8R"(ꬵ)", u8R"(ρ)",    u8R"(Ħ)",
            u8R"(Ҥ)",  u8R"(ģ)",    u8R"(𦖋)",   u8R"(㱏)",   u8R"(ሐ)",   u8R"(4)", u8R"(=)",
        });

        const auto string = std::reduce(data.begin(), data.end(), std::u8string{});
        std::vector<bc::U8Unit> codepoints{};
        std::transform(data.begin(), data.end(), std::back_inserter(codepoints), [](auto &&str) {
            return bc::first_codepoint(str);
        });

        CHECK(std::equal(UTF8Iterator(string), UTF8Iterator::end(string), codepoints.cbegin()));
    }
}

TEST_CASE("as_utf8 / as_ascii", "[src]")
{
    constexpr auto *orig = u8R"(🮕🖜🞭📎🸘🴆🄧🂟🂰🖷🴚🎣👒🹓🱸🈪🗐🌦🋡)";
    CHECK(bc::as_utf8(bc::as_ascii(orig)) == orig);
}

TEST_CASE("str_compare", "[src]")
{
    using bc::str_compare;

    SECTION("Basic ASCII")
    {
        STATIC_REQUIRE_FALSE(str_compare(u8"A", u8"a"));
        CHECK(str_compare(u8"A", u8"a", false)); // could be constexpr but msvc doesn't want it

        STATIC_REQUIRE_FALSE(str_compare(u8"somepath/c/x/d!", u8"somepath/C/X/D!"));
        CHECK(str_compare(u8"somepath/c/x/d!", u8"somepath/C/X/D!", false));
    }
}

TEST_CASE("str_find", "[src]")
{
    using bc::str_find;

    STATIC_REQUIRE(str_find(u8"abcdÀ👒<f¹øì►", u8"à👒") == std::string::npos);
    STATIC_REQUIRE(str_find(u8"abcdÀ👒<f¹øì►", u8"À👒") == 4);
    STATIC_REQUIRE(str_find(u8"abcdÀ👒<f¹øì►", u8"à👒", false) == 4);
}

TEST_CASE("str_contain", "[src]")
{
    using bc::str_contain;

    STATIC_REQUIRE_FALSE(str_contain(u8"abcdÀ👒<f¹øì►", u8"à👒"));
    STATIC_REQUIRE(str_contain(u8"abcdÀ👒<f¹øì►", u8"à👒", false));
}

TEST_CASE("to_lower", "[src]")
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

TEST_CASE("first_codepoint", "[src]")
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

TEST_CASE("concat_codepoint", "[src]")
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

TEST_CASE("str_match", "[src]")
{
    using btu::common::str_match;

    SECTION("Basic")
    {
        STATIC_REQUIRE(str_match(u8"geeks", u8"g*ks"));
        STATIC_REQUIRE(str_match(u8"geeksforgeeks", u8"ge?ks*"));
        STATIC_REQUIRE(str_match(u8"abcdhghgbcd", u8"abc*bcd"));
        STATIC_REQUIRE(str_match(u8"abcd", u8"*c*d"));
        STATIC_REQUIRE(str_match(u8"abcd", u8"*?c*d"));
        STATIC_REQUIRE(str_match(u8"abcd", u8"*?*?c*d"));
        STATIC_REQUIRE(str_match(u8"", u8""));
        STATIC_REQUIRE(str_match(u8"", u8"*"));
        STATIC_REQUIRE(str_match(u8"a", u8"[abc]"));
        STATIC_REQUIRE(str_match(u8"abcd", u8"*?*?[dc]*d"));
        STATIC_REQUIRE(str_match(u8"aa*a", u8"aa[*]a"));

        STATIC_REQUIRE_FALSE(str_match(u8"pqrst", u8"*pqrs"));
        STATIC_REQUIRE_FALSE(str_match(u8"gee", u8"g*k"));
        STATIC_REQUIRE_FALSE(str_match(u8"abcd", u8"abc*c?d"));
        STATIC_REQUIRE_FALSE(str_match(u8"", u8"?"));
        STATIC_REQUIRE_FALSE(str_match(u8"s", u8"[abc]"));
        STATIC_REQUIRE_FALSE(str_match(u8"a_aa ", u8"[ab][ab]*"));
    }
    SECTION("Case sensitivity", "[src]")
    {
        STATIC_REQUIRE(str_match(u8"geEksforgeeks", u8"ge?ks*"));
        STATIC_REQUIRE(str_match(u8"ABCD", u8"*c*d", false));

        STATIC_REQUIRE_FALSE(str_match(u8"geeks", u8"G*ks"));
    }
    SECTION("Set", "[src]")
    {
        STATIC_REQUIRE(str_match(u8"c", u8"[abc]"));
        STATIC_REQUIRE_FALSE(str_match(u8"c", u8"[ab]"));
    }
    SECTION("paths", "[src]")
    {
        constexpr auto path = u8"E:/Documents/SomeData/SomeFolder/file.dds";
        STATIC_REQUIRE(str_match(path, u8"*.dds"));
        STATIC_REQUIRE(str_match(path, u8"e:/*", false));
        STATIC_REQUIRE(str_match(path, u8"E:/*/SomeFolder/*.*"));

        STATIC_REQUIRE_FALSE(str_match(path, u8"E:/*/SomeFolder/*.bsa"));

        STATIC_REQUIRE(str_match(u8"textures/hello.tga", u8"*[s]/*.[td][gd][as]"));
        STATIC_REQUIRE(str_match(u8"textures/my/world/is/purple/hello.dds", u8"*[s]/*.[td][gd][as]"));
    }
    SECTION("malformed input", "[src]")
    {
        STATIC_REQUIRE_FALSE(str_match(u8"abc", u8"["));
        STATIC_REQUIRE_FALSE(str_match(u8"abc", u8"]"));
        STATIC_REQUIRE_FALSE(str_match(u8"abc", u8"]"));
        STATIC_REQUIRE_FALSE(str_match(u8"abc", u8"[[[abc]]]"));
    }
}

TEST_CASE("make_valid", "[src]")
{
    using btu::common::make_valid;

    const auto data = std::to_array<std::pair<std::u8string, std::u8string>>({
        {u8"valid text ᔔ", u8"valid text ᔔ"},
        {{234, 238, 239, 232, 255}, u8"_____"},
    });

    for (const auto &[input, expect] : data)
    {
        std::u8string in = input;
        CHECK(make_valid(in, '_') == expect);
    }
}
