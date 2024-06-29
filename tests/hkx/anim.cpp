#include "btu/hkx/anim.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"
#include "btu/common/games.hpp"

#include <catch.hpp>

#include <filesystem>
#include <thread>

const Path k_dir     = "convert-hkx";
const Path k_exe_dir = "exe";

class HkxTempPath : public TempPath
{
public:
    HkxTempPath()
        : TempPath(k_dir, u8".hkx")
    {
    }
};

[[nodiscard]] auto make_exe() noexcept -> btu::hkx::AnimExe
{
    auto exe = btu::hkx::AnimExe::make(k_exe_dir);
    return require_expected(std::move(exe));
}

/// Does both on disk and in memory
void convert(btu::Game target_game, const Path &input, const Path &output)
{
    const auto exe = make_exe();

    require_expected(exe.convert(target_game, input, output));

    const auto out      = HkxTempPath();
    const auto in_data  = btu::common::read_file(input);
    const auto out_data = require_expected(exe.convert(target_game, in_data));

    btu::common::write_file(out.path(), out_data);
    CHECK(btu::common::compare_files(output, out.path()));
}

TEST_CASE("Converting from LE to SE", "[src]")
{
    const auto out = HkxTempPath();
    convert(btu::Game::SSE, k_dir / "LE_INPUT.hkx", out.path());
    CHECK(btu::common::compare_files(k_dir / "LE_EXPECTED.hkx", out.path()));
}

TEST_CASE("Converting from SE to LE", std::string("[src]") + std::string(k_fail_on_linux_tag))
{
    const auto out = HkxTempPath();
    convert(btu::Game::SLE, k_dir / "SE_INPUT.hkx", out.path());
    CHECK(btu::common::compare_files(k_dir / "SE_EXPECTED.hkx", out.path()));
}

TEST_CASE("Trying to convert SE to SE", "[src]")
{
    const auto exe = make_exe();
    const auto out = HkxTempPath();
    auto res       = exe.convert(btu::Game::SSE, k_dir / "SE_INPUT.hkx", out.path());

    REQUIRE_FALSE(res);
    REQUIRE_FALSE(exists(k_dir / "SE_OUTPUT2.hkx"));
}

TEST_CASE("Handles parallelism", "[src]")
{
    std::vector<HkxTempPath> outputs(5);

    flux::ref(outputs)
        .map([](const HkxTempPath &out) {
            return std::jthread([&out] { convert(btu::Game::SSE, k_dir / "LE_INPUT.hkx", out.path()); });
        })
        .to<std::vector<std::jthread>>();

    for (const auto &out : outputs)
        CHECK(btu::common::compare_files(k_dir / "LE_EXPECTED.hkx", out.path()));
}
