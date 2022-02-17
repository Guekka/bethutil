/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/hkx/functions.hpp"

#include "btu/common/string.hpp"
#include "btu/hkx/anim.hpp"

#include <btu/common/path.hpp>
#include <reproc++/run.hpp>

#include <atomic>
#include <filesystem>
#include <string>

namespace btu::hkx {
using namespace std::literals;

auto run_process(const std::vector<std::string> &args, const Path &working_dir) -> ResultError
{
    reproc::options options;
    options.deadline        = std::chrono::milliseconds(5000);
    options.redirect.parent = true;

    const auto wdir           = btu::common::as_ascii_string(working_dir.u8string());
    options.working_directory = wdir.c_str();

    auto [result, ec] = reproc::run(args, options);
    if (ec)
        return tl::make_unexpected(Error(ec));
    if (result != 0)
        return tl::make_unexpected(Error(std::error_code(result, std::generic_category())));
    return {};
}

auto convert(Anim &anim, const AnimExe &exe, btu::Game target_game) -> ResultError
{
    const auto &exe_dir = exe.get_directory();
    const auto args     = [&]() noexcept -> std::vector<std::string> {
        switch (target_game)
        {
            case btu::Game::TES3:
            case btu::Game::TES4:
            case btu::Game::FNV:
            case btu::Game::FO4:
            case btu::Game::Custom: return {};
            case btu::Game::SLE:
                return {
                    (exe_dir / AnimExe::exe64to32).string(),
                    anim.get().filename().string(),
                    "-s"s,
                    "32ref.hko"s,
                };

            case btu::Game::SSE:
                return {
                    (exe_dir / AnimExe::exe32to64).string(),
                    anim.get().filename().string(),
                };
        }
        return {};
    }();

    if (args.empty())
        return tl::make_unexpected(Error(std::error_code(1, std::generic_category())));

    return run_process(args, exe_dir);
}

} // namespace btu::hkx
