#include "btu/hkx/anim.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"
#include "btu/common/games.hpp"

#include <catch.hpp>

#include <filesystem>

const Path k_dir     = "convert-hkx";
const Path k_exe_dir = "exe";

void convert(btu::Game target_game, const Path &input, const Path &output)
{
    const auto exe = btu::hkx::AnimExe::make(k_exe_dir);
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
    convert(btu::Game::SSE, k_dir / "LE_INPUT.hkx", k_dir / "LE_OUTPUT.hkx");
    CHECK(btu::common::compare_files(k_dir / "LE_EXPECTED.hkx", k_dir / "LE_OUTPUT.hkx"));
}

TEST_CASE("Converting from SE to LE", std::string("[src]") + std::string(k_fail_on_linux_tag))
{
    convert(btu::Game::SLE, k_dir / "SE_INPUT.hkx", k_dir / "SE_OUTPUT.hkx");
    CHECK(btu::common::compare_files(k_dir / "SE_EXPECTED.hkx", k_dir / "SE_OUTPUT.hkx"));
}
