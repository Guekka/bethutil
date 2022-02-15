/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/hkx/anim.hpp"

namespace btu::hkx {
auto AnimExe::make(Path dir) noexcept -> tl::expected<AnimExe, Error>
{
    if (fs::exists(dir / exe32to64) || fs::exists(dir / exe64to32))
    {
        AnimExe ae{};
        ae.dir_ = std::move(dir);
        return ae;
    }
    return tl::make_unexpected(Error(std::error_code(1, std::generic_category())));
}

auto AnimExe::get_directory() const noexcept -> const Path &
{
    return dir_;
}

auto generate_name() noexcept -> std::string
{
    static std::atomic_uint32_t counter{0};
    return "TEMPFILE_btu__hkx" + std::to_string(counter++) + ".hkx ";
}

auto Anim::get() const noexcept -> const Path &
{
    return path_;
}

void Anim::set(Path path) noexcept
{
    path_ = std::move(path);
}

auto Anim::get_load_path() const noexcept -> const Path &
{
    return load_path_;
}

void Anim::set_load_path(Path path) noexcept
{
    load_path_ = std::move(path);
}

tl::expected<Anim, Error> load(Path path, AnimExe exe) noexcept
{
    const auto new_path = exe.get_directory() / generate_name();
    std::error_code ec;
    fs::remove(new_path, ec);
    if (ec)
        return tl::make_unexpected(Error(ec));
    fs::copy(path, new_path, ec);
    if (ec)
        return tl::make_unexpected(Error(ec));

    Anim a;
    a.set(std::move(new_path));
    a.set_load_path(std::move(path));
    return a;
}

ResultError save(const Anim &anim, const Path &path) noexcept
{
    std::error_code ec;
    fs::remove(path, ec);
    if (ec)
        return tl::make_unexpected(Error(ec));
    fs::copy(anim.get(), path, ec);
    if (ec)
        return tl::make_unexpected(Error(ec));
    return {};
}

} // namespace btu::hkx
