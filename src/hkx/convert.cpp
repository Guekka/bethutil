/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/hkx/convert.hpp"

#include "btu/common/string.hpp"

#include <btu/common/path.hpp>
#include <reproc++/run.hpp>

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>

namespace btu::hkx {
using btu::common::Path;
namespace fs = std::filesystem;

using namespace std::literals;

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

auto Anim::generate_name() const noexcept -> std::string
{
    static std::atomic_uint32_t counter{0};
    return "TEMPFILE_btu__hkx" + std::to_string(counter++) + ".hkx ";
}

auto Anim::load(const std::filesystem::path &path) noexcept -> std::error_code
{
    path_ = exe_dir_ / generate_name();
    std::error_code ec;
    std::filesystem::remove(path_, ec);
    if (ec)
        return ec;
    std::filesystem::copy(path, path_, ec);
    return ec;
}

auto Anim::save(const std::filesystem::path &path) noexcept -> std::error_code
{
    std::error_code ec;
    std::filesystem::copy(path_, path, ec);
    return ec;
}

auto Anim::sle_args() const noexcept -> std::vector<std::string>
{
    return {(exe_dir_ / "hkx64to32.exe").string(), path_.filename().string(), "-s"s, "32ref.hko"s};
}

auto Anim::sse_args() const noexcept -> std::vector<std::string>
{
    return {(exe_dir_ / "hkx32to64.exe").string(), path_.filename().string()};
}

auto Anim::convert(btu::common::Game target_game) -> std::error_code
{
    const auto args = [&]() noexcept -> std::vector<std::string> {
        switch (target_game)
        {
            case btu::common::Game::TES3:
            case btu::common::Game::TES4:
            case btu::common::Game::FNV:
            case btu::common::Game::FO4:
            case btu::common::Game::Custom: return {};
            case btu::common::Game::SLE: return sle_args();
            case btu::common::Game::SSE: return sse_args();
        }
        return {};
    }();

    if (args.empty())
        return std::error_code(1, std::generic_category());

    return run_process(args, exe_dir_);
}

} // namespace btu::hkx
