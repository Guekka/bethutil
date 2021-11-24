/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/hkx/convert.hpp"

#include "btu/common/string.hpp"

#include <btu/common/path.hpp>
#include <reproc++/run.hpp>

#include <filesystem>

namespace btu::hkx {
using btu::common::Path;
namespace fs = std::filesystem;

using namespace std::literals;

static constexpr auto k_temp_filename = "TEMPFILE_btu__hkx.hkx";

auto run_process(const std::vector<std::string> &args, const Path &working_dir) -> std::error_code
{
    reproc::options options;
    options.deadline        = std::chrono::milliseconds(5000);
    options.redirect.parent = true;

    const auto wdir           = btu::common::as_ascii_string(working_dir.u8string());
    options.working_directory = wdir.c_str();

    auto [result, ec] = reproc::run(args, options);
    return ec ? ec : std::error_code(result, std::generic_category());
}

auto load(const std::filesystem::path &path, const std::filesystem::path &exe_dir) noexcept -> std::error_code
{
    std::error_code ec;
    std::filesystem::remove(exe_dir / k_temp_filename, ec);
    if (ec)
        return ec;
    std::filesystem::copy(path, exe_dir / k_temp_filename, ec);
    return ec;
}

auto save(const std::filesystem::path &path, const std::filesystem::path &exe_dir) noexcept -> std::error_code
{
    std::error_code ec;
    std::filesystem::copy(exe_dir / k_temp_filename, path, ec);
    return ec;
}

auto sle_args(const Path &exe_dir) noexcept -> std::vector<std::string>
{
    return {(exe_dir / "hkx64to32.exe").string(), k_temp_filename, "-s"s, "32ref.hko"s};
}

auto sse_args(const Path &exe_dir) noexcept -> std::vector<std::string>
{
    return {(exe_dir / "hkx32to64.exe").string(), k_temp_filename};
}

auto convert(btu::common::Game target_game, const Path &exe_dir) -> std::error_code
{
    const auto args = [&]() noexcept -> std::vector<std::string> {
        switch (target_game)
        {
            case btu::common::Game::TES3:
            case btu::common::Game::TES4:
            case btu::common::Game::FNV:
            case btu::common::Game::FO4:
            case btu::common::Game::Custom: return {};
            case btu::common::Game::SLE: return sle_args(exe_dir);
            case btu::common::Game::SSE: return sse_args(exe_dir);
        }
        return {};
    }();

    if (args.empty())
        return std::error_code(1, std::generic_category());

    return run_process(args, exe_dir);
}

} // namespace btu::hkx
