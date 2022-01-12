/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#define _SILENCE_CLANG_COROUTINE_MESSAGE
#include "btu/bsa/archive.hpp"
#include "btu/common/path.hpp"

#include <flow.hpp>
#include <neo/iterator_facade.hpp>

#include <variant>

namespace btu::modmanager {
using btu::common::Path;

namespace detail {
struct ModFileDisk
{
    Path file_path;
};

} // namespace detail

class ModFile
{
public:
    ModFile() = default;

    ModFile(std::variant<btu::bsa::Archive::Iterator, detail::ModFileDisk> content)
        : file_(std::move(content))
    {
    }

    auto is_on_disk() -> bool;
    auto get_or_write(const Path *out) -> const Path &;

    auto &get_underlying() { return file_; }

private:
    std::variant<btu::bsa::Archive::Iterator, detail::ModFileDisk> file_;
};

class ModFolder
{
public:
    explicit ModFolder(Path directory, std::u8string archive_ext);

    [[nodiscard]] auto size() -> size_t;

    class Iterator;
    class Sentinel
    {
    };

    auto dereference() { return; }

    [[nodiscard]] auto begin() -> Iterator;
    [[nodiscard]] auto end() -> Sentinel;

private:
    std::vector<std::unique_ptr<btu::bsa::Archive>> archives_;
    std::vector<ModFile> loose_files_;

    size_t count_;

    Path dir_;
    std::u8string archive_ext_;
};

class ModFolder::Iterator : public neo::iterator_facade<ModFolder::Iterator>
{
    static constexpr auto init = [](ModFolder &mf) {
        return flow::from(mf.archives_)
            .flat_map([](auto &a) { return flow::from(*a); })
            .map([](const std::string &) { return ModFile{}; })
            .chain(flow::copy(mf.loose_files_));
    };

    using IteratorType = std::invoke_result_t<decltype(init), ModFolder &>;
    using ValueType    = flow::next_t<IteratorType>;

    static constexpr bool single_pass_iterator = true;

public:
    Iterator(ModFolder &mf) noexcept
        : it_(init(mf))
    {
        increment();
    }

    auto increment() -> Iterator &
    {
        val_ = it_.next();
        return *this;
    }

    auto dereference() const -> ValueType { return val_; }

    auto operator==(ModFolder::Sentinel) -> bool;

private:
    IteratorType it_;
    ValueType val_;
};

static_assert(std::input_iterator<ModFolder::Iterator>);

} // namespace btu::modmanager
