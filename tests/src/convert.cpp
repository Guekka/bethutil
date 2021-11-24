#include "btu/common/games.hpp"

#include <btu/hkx/convert.hpp>
#include <catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using btu::hkx::load, btu::hkx::save, btu::hkx::convert;

const fs::path dir     = "convert";
const fs::path exe_dir = "exe";

bool compare_files(const fs::path &filename1, const fs::path &filename2)
{
    std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary); //open file at the end
    std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary); //open file at the end

    if (file1.tellg() != file2.tellg())
    {
        return false; //different file size
    }

    file1.seekg(0); //rewind
    file2.seekg(0); //rewind

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    return std::equal(begin1,
                      std::istreambuf_iterator<char>(),
                      begin2); //Second argument is end-of-range iterator
}

TEST_CASE("Converting from LE to SE")
{
    REQUIRE_FALSE(load(dir / "LE_INPUT.hkx", exe_dir));
    REQUIRE_FALSE(convert(btu::common::Game::SSE, exe_dir));
    std::filesystem::remove(dir / "LE_OUTPUT.hkx");
    REQUIRE_FALSE(save(dir / "LE_OUTPUT.hkx", exe_dir));
    CHECK(compare_files(dir / "LE_EXPECTED.hkx", dir / "LE_OUTPUT.hkx"));
}

TEST_CASE("Converting from SE to LE")
{
    REQUIRE_FALSE(load(dir / "SE_INPUT.hkx", exe_dir));
    REQUIRE_FALSE(convert(btu::common::Game::SLE, exe_dir));
    std::filesystem::remove(dir / "SE_OUTPUT.hkx");
    REQUIRE_FALSE(save(dir / "SE_OUTPUT.hkx", exe_dir));
    CHECK(compare_files(dir / "SE_EXPECTED.hkx", dir / "SE_OUTPUT.hkx"));
}
