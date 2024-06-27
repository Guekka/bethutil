/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/plugin.hpp"

#include "btu/common/algorithms.hpp"

#include <fstream>
#include <ios>

namespace btu::bsa {
FilePath::FilePath(Path dir, std::u8string name, std::u8string suffix, std::u8string ext, FileTypes type)
    : dir(std::move(dir))
    , name(std::move(name))
    , suffix(std::move(suffix))
    , ext(std::move(ext))
    , type(type)
{
}

auto FilePath::make(const Path &path, const Settings &sets, FileTypes type) -> std::optional<FilePath>
{
    if (is_directory(path))
        return std::nullopt;

    FilePath file(path.parent_path(), path.stem().u8string(), {}, path.extension().u8string(), type);

    if (type == FileTypes::Plugin && !common::contains(sets.plugin_extensions, file.ext))
        return std::nullopt;

    if (type == FileTypes::BSA && file.ext != sets.extension)
        return std::nullopt;

    file.counter = eat_digits(file.name);
    file.suffix  = eat_suffix(file.name, sets);

    if (!file.counter.has_value())
        file.counter = eat_digits(file.name);

    return file;
}

auto FilePath::full_path() const -> Path
{
    return dir / (full_name() + ext);
}

auto FilePath::full_name() const -> std::u8string
{
    const auto count = counter ? common::as_utf8_string(std::to_string(*counter)) : u8"";
    const auto suf   = suffix.empty() ? u8"" : suffix_separator + suffix;
    return {name + count + suf};
}

auto FilePath::eat_digits(std::u8string &str) -> std::optional<uint32_t>
{
    size_t first_digit = str.length() - 1;
    for (; isdigit(str[first_digit]) != 0; --first_digit)
        ;
    ++first_digit;

    if (first_digit != str.length())
    {
        std::optional<uint32_t> ret{};
        try
        {
            ret = std::stoul(common::as_ascii_string(str.substr(first_digit)));
        }
        catch (const std::exception &)
        {
            return std::nullopt;
        }

        str.erase(first_digit);
        return ret;
    }
    return std::nullopt;
}

auto FilePath::eat_suffix(std::u8string &str, const Settings &sets) -> std::u8string
{
    const auto suffix_pos = str.rfind(suffix_separator);

    if (suffix_pos == std::u8string::npos)
        return {};

    auto suffix = str.substr(suffix_pos + suffix_separator.length());
    if (suffix != sets.suffix && suffix != sets.texture_suffix)
        return {};

    str.erase(suffix_pos);
    return suffix;
}

auto is_loaded(const FilePath &archive, const Settings &sets) -> bool
{
    return std::ranges::any_of(sets.plugin_extensions, [&archive](const auto &ext) {
        auto b            = archive;
        b.ext             = ext;
        bool const exact  = exists(b.full_path());
        b.suffix          = {};
        bool const approx = exists(b.full_path());
        return exact || approx;
    });
}

auto find_archive_name(std::span<const FilePath> plugins, const Settings &sets, ArchiveType type) -> FilePath
{
    if (plugins.empty())
        throw std::invalid_argument("Plugins cannot be empty");

    const std::u8string suffix = [type, &sets] {
        if (type == ArchiveType::Textures)
        {
            return sets.texture_suffix.value_or(u8"");
        }
        return sets.suffix.value_or(u8"");
    }();

    auto check_plugin = [&sets, &suffix](FilePath &file) {
        file.ext    = sets.extension;
        file.suffix = suffix;
        return !exists(file.full_path());
    };

    for (auto plugin : plugins)
        if (check_plugin(plugin))
            return plugin;

    FilePath plug                    = plugins.front();
    constexpr uint8_t max_iterations = UINT8_MAX;
    for (plug.counter = 0; plug.counter < max_iterations; ++*plug.counter)
        if (check_plugin(plug))
            return plug;

    throw std::runtime_error("No btu/bsa/plugin name found after 256 tries.");
}

void clean_dummy_plugins(std::vector<FilePath> &plugins, const Settings &sets)
{
    if (!sets.s_dummy_plugin.has_value())
        return;
    const auto &dummy = *sets.s_dummy_plugin;

    auto is_dummy = [&dummy](const FilePath &f) {
        std::fstream file(f.full_path(), std::ios::binary | std::ios::in | std::ios::ate);

        // It is safe to evaluate file size, as the embedded dummies are the smallest plugins possible
        return file && file.tellg() == static_cast<std::streamoff>(dummy.size());
    };

    auto dummies = std::ranges::stable_partition(plugins, std::not_fn(is_dummy));

    std::ranges::for_each(dummies, [](const auto &f) { fs::remove(f.full_path()); });
    // remove dummy plugins from plugins
    plugins.erase(dummies.begin(), dummies.end());
}

void make_dummy_plugins(std::span<const FilePath> archives, const Settings &sets)
{
    if (!sets.s_dummy_plugin.has_value())
        return;

    for (auto &&bsa : archives)
    {
        if (is_loaded(bsa, sets))
            continue;

        auto mut_bsa   = bsa;
        mut_bsa.ext    = sets.dummy_extension;
        mut_bsa.suffix = {};
        std::ofstream dummy(mut_bsa.full_path(), std::ios::out | std::ios::binary);

        const auto dummy_bytes = *sets.s_dummy_plugin;
        dummy.write(reinterpret_cast<const char *>(dummy_bytes.data()),
                    static_cast<std::streamsize>(dummy_bytes.size()));
    }
}

} // namespace btu::bsa
