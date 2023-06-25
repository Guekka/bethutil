#include "btu/hkx/anim.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"
#include "btu/common/games.hpp"

#include <catch.hpp>

#include <filesystem>

const Path dir     = "convert-hkx";
const Path exe_dir = "exe";

void convert(btu::Game target_game, const Path &input, const Path &output)
{
    const auto exe = btu::hkx::AnimExe::make(exe_dir);
    if (!exe)
    {
        INFO(exe.error());
        FAIL_CHECK();
    }

    auto res = exe->convert(target_game, input, output);

    if (!res)
    {
        INFO(res.error());
        FAIL_CHECK();
    }
}

TEST_CASE("Converting from LE to SE", "[src]")
{
    convert(btu::Game::SSE, dir / "LE_INPUT.hkx", dir / "LE_OUTPUT.hkx");
    CHECK(btu::common::compare_files(dir / "LE_EXPECTED.hkx", dir / "LE_OUTPUT.hkx"));
}

TEST_CASE("Converting from SE to LE", "[src]")
{
    convert(btu::Game::SLE, dir / "SE_INPUT.hkx", dir / "SE_OUTPUT.hkx");
    CHECK(btu::common::compare_files(dir / "SE_EXPECTED.hkx", dir / "SE_OUTPUT.hkx"));
}
