#include "../utils.hpp"
#include "btu/common/filesystem.hpp"
#include "btu/common/games.hpp"
#include "btu/hkx/anim.hpp"

#include <btu/hkx/functions.hpp>
#include <catch.hpp>

#include <filesystem>
#include <fstream>

const Path dir     = "convert-hkx";
const Path exe_dir = "exe";

TEST_CASE("Converting from LE to SE", "[src]")
{
    auto exe = btu::hkx::AnimExe::make(exe_dir);
    REQUIRE(exe);

    auto anim = btu::hkx::load(dir / "LE_INPUT.hkx", *exe);
    REQUIRE(anim.has_value());
    REQUIRE(btu::hkx::convert(*anim, *exe, btu::Game::SSE).has_value());
    REQUIRE(btu::hkx::save(*anim, dir / "LE_OUTPUT.hkx").has_value());
    CHECK(btu::common::compare_files(dir / "LE_EXPECTED.hkx", dir / "LE_OUTPUT.hkx"));
}

TEST_CASE("Converting from SE to LE", "[src]")
{
    auto exe = btu::hkx::AnimExe::make(exe_dir);
    REQUIRE(exe);

    auto anim = btu::hkx::load(dir / "SE_INPUT.hkx", *exe);
    REQUIRE(anim.has_value());
    REQUIRE(btu::hkx::convert(*anim, *exe, btu::Game::SLE).has_value());
    REQUIRE(btu::hkx::save(*anim, dir / "SE_OUTPUT.hkx").has_value());
    CHECK(btu::common::compare_files(dir / "SE_EXPECTED.hkx", dir / "SE_OUTPUT.hkx"));
}
