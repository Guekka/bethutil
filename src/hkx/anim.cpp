/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/common/string.hpp"
#include "tl/expected.hpp"

#include <btu/hkx/anim.hpp>
#include <btu/hkx/error_code.hpp>
#include <flow.hpp>
#include <reproc++/reproc.hpp>
#include <reproc++/run.hpp>

#include <atomic>
#include <filesystem>

using namespace std::literals;

namespace btu::hkx {
class Info32to64 : public detail::AnimExeInfo
{
public:
    [[nodiscard]] constexpr auto name() const noexcept -> std::string_view override
    {
        return "hkx32to64.exe";
    }
    [[nodiscard]] constexpr auto input_file_name() const noexcept -> std::string_view override
    {
        return "input.hkx";
    }
    [[nodiscard]] constexpr auto output_file_name() const noexcept -> std::string_view override
    {
        return "OUTFILE64.hkx";
    }
    [[nodiscard]] constexpr auto target_game() const noexcept -> btu::Game override { return btu::Game::SSE; }

    [[nodiscard]] auto get_full_args(const Path &exe_dir) const -> std::vector<std::string> override
    {
        return {(exe_dir / name()).string(), std::string(input_file_name())};
    }
};
static inline const auto k_exe_32to64 = Info32to64{};

class Info64to32 : public detail::AnimExeInfo
{
public:
    [[nodiscard]] constexpr auto name() const noexcept -> std::string_view override
    {
        return "hkx64to32.exe";
    }
    [[nodiscard]] constexpr auto input_file_name() const noexcept -> std::string_view override
    {
        return "input.hkx";
    }
    [[nodiscard]] constexpr auto output_file_name() const noexcept -> std::string_view override
    {
        return "OUTFILE32.hkx";
    }
    [[nodiscard]] constexpr auto target_game() const noexcept -> btu::Game override { return btu::Game::SLE; }

    [[nodiscard]] auto get_full_args(const Path &exe_dir) const -> std::vector<std::string> override
    {
        return {(exe_dir / name()).string(), std::string(input_file_name())};
    }
};
static inline const auto k_exe_64to32 = Info64to32{};

static inline const auto k_exe_list = std::to_array<detail::AnimExeRef>(
    {std::cref(k_exe_32to64), std::cref(k_exe_64to32)});

auto AnimExe::make(Path exe_dir) noexcept -> tl::expected<AnimExe, Error>
{
    auto detected = flow::from(k_exe_list)
                        .filter([&exe_dir](auto info) noexcept {
                            return fs::exists(exe_dir / info.get().name());
                        })
                        .to_vector();

    if (detected.empty())
        return tl::make_unexpected(Error(AnimErr::NoExeFound));

    return AnimExe{std::move(exe_dir), std::move(detected)};
}

[[nodiscard]] auto make_working_dir() noexcept -> tl::expected<Path, Error>
{
    static std::atomic<uint32_t> counter{0};
    const auto dir_name = "TEMPDIR_btu__hkx" + std::to_string(counter++);
    const auto dir_path = fs::temp_directory_path() / dir_name;

    auto ec = std::error_code{};
    fs::create_directory(dir_path, ec);

    if (ec)
        return tl::make_unexpected(Error(ec));

    return dir_path;
}

[[nodiscard]] auto reproc(const std::vector<std::string> &args, const reproc::options &options)
    -> tl::expected<int, Error>
{
    for (const auto &arg : args)
        std::cout << arg << std::endl;
    auto [result, ec] = reproc::run(args, options);
    if (ec)
        return tl::make_unexpected(Error(ec));

    return result;
}

[[nodiscard]] auto find_appropriate_exe(const std::vector<detail::AnimExeRef> &detected,
                                        btu::Game target_game) -> tl::expected<detail::AnimExeRef, Error>
{
    const auto it = std::find_if(detected.begin(), detected.end(), [target_game](auto info) {
        return info.get().target_game() == target_game;
    });

    if (it == detected.end())
        return tl::make_unexpected(Error(AnimErr::NoAppropriateExe));

    return *it;
}

struct ReprocOptions : public reproc::options
{
    std::string working_directory_storage; // working_directory expects a pointer. This allows us to use RAII
};

[[nodiscard]] auto make_reproc_options(const Path &working_dir) -> ReprocOptions
{
    ReprocOptions options;
    options.deadline                  = std::chrono::milliseconds(5000);
    options.redirect.parent           = true;
    options.working_directory_storage = btu::common::as_ascii_string(working_dir.u8string());
    options.working_directory         = options.working_directory_storage.c_str();
    return options;
}

[[nodiscard]] auto copy_file_to_input(const Path &input,
                                      const Path &working_dir,
                                      const detail::AnimExeRef exe_info) -> ResultError
{
    const auto input_path = working_dir / exe_info.get().input_file_name();

    auto ec = std::error_code{};
    fs::remove(input_path, ec); // leftover from previous runs
    if (ec)
        return tl::make_unexpected(Error(ec));

    fs::copy(input, input_path, ec);
    if (ec)
        return tl::make_unexpected(Error(ec));

    return {};
}

[[nodiscard]] auto move_output_to_file(const Path &working_dir,
                                       const detail::AnimExeRef exe_info,
                                       const Path &output) -> ResultError
{
    const auto output_path = working_dir / exe_info.get().output_file_name();

    auto ec = std::error_code{};

    fs::remove(output, ec);
    if (ec)
        return tl::make_unexpected(Error(ec));

    fs::rename(output_path, output, ec);
    if (ec)
        return tl::make_unexpected(Error(ec));

    return {};
}

auto AnimExe::convert(btu::Game target_game, const Path &input, const Path &output) const -> ResultError
{
    const auto working_dir = make_working_dir();
    auto options           = working_dir.map(make_reproc_options);
    if (!options)
        return tl::make_unexpected(options.error());

    const auto exe = find_appropriate_exe(detected_, target_game);
    if (!exe)
        return tl::make_unexpected(exe.error());

    const auto copy = copy_file_to_input(input, *working_dir, *exe);
    if (!copy)
        return tl::make_unexpected(copy.error());

    const auto args = exe->get().get_full_args(exe_dir_);

    return reproc(args, *options)
        .and_then([](int result) noexcept -> ResultError {
            return result == 0 ? ResultError{} : tl::make_unexpected(Error(AnimErr::ExeFailed));
        })
        .and_then([&]() noexcept -> ResultError { return move_output_to_file(*working_dir, *exe, output); });
}

} // namespace btu::hkx
