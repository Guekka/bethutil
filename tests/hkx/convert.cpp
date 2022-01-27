#include "../utils.hpp"
#include "btu/common/filesystem.hpp"
#include "btu/common/games.hpp"

#include <btu/hkx/convert.hpp>
#include <catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

const fs::path dir     = "convert-hkx";
const fs::path exe_dir = "exe";

TEST_CASE("Converting from LE to SE", "[src]")
{
    btu::hkx::Anim anim(exe_dir);
    REQUIRE_FALSE(anim.load(dir / "LE_INPUT.hkx"));
    REQUIRE_FALSE(anim.convert(btu::common::Game::SSE));
    std::filesystem::remove(dir / "LE_OUTPUT.hkx");
    REQUIRE_FALSE(anim.save(dir / "LE_OUTPUT.hkx"));
    CHECK(btu::common::compare_files(dir / "LE_EXPECTED.hkx", dir / "LE_OUTPUT.hkx"));
}

TEST_CASE("Converting from SE to LE", "[src]")
{
    btu::hkx::Anim anim(exe_dir);
    REQUIRE_FALSE(anim.load(dir / "SE_INPUT.hkx"));
    REQUIRE_FALSE(anim.convert(btu::common::Game::SLE));
    std::filesystem::remove(dir / "SE_OUTPUT.hkx");
    REQUIRE_FALSE(anim.save(dir / "SE_OUTPUT.hkx"));
    CHECK(btu::common::compare_files(dir / "SE_EXPECTED.hkx", dir / "SE_OUTPUT.hkx"));
}
