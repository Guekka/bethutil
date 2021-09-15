/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/archive_data.hpp"

#include "btu/bsa/pack.hpp"
#include "utils.hpp"

#include <random>

using namespace btu::bsa;

ArchiveData make_arch(uintmax_t max, uintmax_t size, ArchiveType type, uint32_t file_count = 1)
{
    auto sets    = Settings::get(Game::SSE);
    sets.maxSize = max;
    auto arch    = ArchiveData(sets, type);

    REQUIRE(arch.add_file("", ArchiveData::Size{size, size}));
    for (uint32_t i = 1; i < file_count; ++i)
        REQUIRE(arch.add_file("", ArchiveData::Size{0, 0}));

    return arch;
}

TEST_CASE("Full archives are not merged")
{
    auto input = std::vector{
        make_arch(1'000, 500, ArchiveType::Standard),
        make_arch(1'000, 600, ArchiveType::Incompressible),
        make_arch(1'000, 700, ArchiveType::Textures),
    };

    const auto expected = input;

    merge(input);

    CHECK(input == expected);
}

TEST_CASE("Testing merge settings")
{
    auto input = std::vector{
        make_arch(1'000, 1, ArchiveType::Standard),
        make_arch(1'000, 2, ArchiveType::Incompressible),
        make_arch(1'000, 4, ArchiveType::Textures),
    };

    auto do_test = [&input](MergeSettings sets, auto expected) {
        merge(input, sets);
        CHECK(input == expected);
    };

    SECTION("MergeBoth")
    {
        const auto expected = std::vector{make_arch(1'000, 7, ArchiveType::Incompressible, 3)};
        do_test(MergeSettings::MergeBoth, expected);
    }
    SECTION("MergeTextures")
    {
        const auto expected = std::vector{
            make_arch(1'000, 5, ArchiveType::Standard, 2),
            make_arch(1'000, 2, ArchiveType::Incompressible),
        };
        do_test(MergeSettings::MergeTextures, expected);
    }
    SECTION("MergeIncompressible")
    {
        const auto expected = std::vector{
            make_arch(1'000, 3, ArchiveType::Incompressible, 2),
            make_arch(1'000, 4, ArchiveType::Textures),
        };
        do_test(MergeSettings::MergeIncompressible, expected);
    }
}
