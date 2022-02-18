/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/bsa/detail/archive_type.hpp"
#include "btu/bsa/settings.hpp"

namespace btu::bsa {
class FilePath
{
    static inline const std::u8string suffix_separator = u8" - ";

public:
    FilePath(Path dir, std::u8string name, std::u8string suffix, std::u8string ext, FileTypes type);
    [[nodiscard]] static auto make(const Path &path, const Settings &sets, FileTypes type)
        -> std::optional<FilePath>;

    [[nodiscard]] auto full_path() const -> Path;
    [[nodiscard]] auto full_name() const -> std::u8string;

    Path dir;
    std::u8string name;
    std::u8string suffix;
    std::u8string ext;
    std::optional<uint32_t> counter;
    FileTypes type{};

private:
    static auto eat_digits(std::u8string &str) -> std::optional<uint32_t>;
    static auto eat_suffix(std::u8string &str, const Settings &sets) -> std::u8string;

    explicit FilePath() = default;
};

[[nodiscard]] auto find_archive_name(std::span<const FilePath> plugins, const Settings &sets, ArchiveType type)
    -> FilePath;

namespace detail {
template<typename It>
auto list_helper(It begin, It end, const Settings &sets, FileTypes type) -> std::vector<FilePath>
{
    std::vector<FilePath> res;
    std::for_each(begin, end, [&](auto &&f) {
        if (auto file = FilePath::make(f.path(), sets, type))
            res.emplace_back(*file);
    });
    return res;
}
} // namespace detail

template<typename It>
auto list_plugins(It begin, It end, const Settings &sets) -> std::vector<FilePath>
{
    return detail::list_helper(begin, end, sets, FileTypes::Plugin);
}

template<typename It>
auto list_archive(It begin, It end, const Settings &sets) -> std::vector<FilePath>
{
    return detail::list_helper(begin, end, sets, FileTypes::BSA);
}

void clean_dummy_plugins(std::vector<FilePath> &plugins, const Settings &sets);
void make_dummy_plugins(std::span<const FilePath> archives, const Settings &sets);
} // namespace btu::bsa
