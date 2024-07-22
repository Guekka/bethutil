/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/plugin.hpp"

#include "btu/common/algorithms.hpp"
#include "btu/common/filesystem.hpp"
#include "btu/common/functional.hpp"

#include <flux.hpp>

#include <fstream>
#include <ios>

namespace btu::bsa {
const auto k_suffix_separator = std::u8string(u8" - ");

[[nodiscard]] auto list_plugins(const Path &dir, const Settings &sets) noexcept -> std::vector<Path>
{
    try
    {
        return flux::from_range(fs::directory_iterator(dir))
            .filter([&sets](const auto &f) {
                return f.is_regular_file()
                       && common::contains(sets.plugin_extensions,
                                           common::to_lower(f.path().extension().u8string()));
            })
            .map([](const auto &f) { return f.path(); })
            .to<std::vector>();
    }
    catch (const std::exception &)
    {
        return {};
    }
}

[[nodiscard]] auto archive_suffixes(const Settings &sets) -> std::vector<std::u8string>
{
    const auto raw_suffixes = std::to_array({sets.suffix, sets.texture_suffix});
    return flux::ref(raw_suffixes)
        .filter_map([](const auto &suffix) {
            return suffix.has_value() ? std::optional{k_suffix_separator + suffix.value()} : std::nullopt;
        })
        .to<std::vector>();
}

[[nodiscard]] auto archive_suffix_with_sep(const Settings &sets, ArchiveType type) -> std::u8string
{
    auto suffix = (type == ArchiveType::Textures ? sets.texture_suffix : sets.suffix).value_or(u8"");
    if (!suffix.empty())
        suffix = k_suffix_separator + suffix;

    return suffix;
}

[[nodiscard]] auto archive_loaded_by_plugin(Path plugin_path, const Settings &sets, ArchiveType type) -> Path
{
    const auto new_filename = plugin_path.stem().u8string() + archive_suffix_with_sep(sets, type)
                              + sets.extension;

    return plugin_path.replace_extension(sets.extension).replace_filename(new_filename);
}

void remove_suffixes(std::u8string &filename, const Settings &sets)
{
    for (const auto &suffix : archive_suffixes(sets))
        filename = str_replace_once(filename, suffix, u8"", common::CaseSensitive::Yes);
}

[[nodiscard]] auto plugins_loading_archive_limited(const Path &archive, const Settings &sets)
{
    auto stem = archive.stem().u8string();
    remove_suffixes(stem, sets);

    return flux::ref(sets.plugin_extensions)
        .map([&archive, &stem](const auto &ext) { return archive.parent_path() / (stem + ext); })
        .to<std::vector>();
}

/// If plugins can have unlimited archives attached, also look at all the prefixes of stem.
[[nodiscard]] auto plugins_loading_archive_unlimited(const Path &archive, const Settings &sets)
{
    const auto stem = archive.stem().u8string();
    return flux::cartesian_product_map([&stem](const size_t size,
                                               const auto &ext) { return stem.substr(0, size) + ext; },
                                       flux::iota(size_t{1}, stem.size() + 1).reverse(),
                                       flux::ref(sets.plugin_extensions))
        .map([&archive](const auto &filename) { return archive.parent_path() / filename; })
        .to<std::vector>();
}

[[nodiscard]] auto plugins_loading_archive(const Path &archive, const Settings &sets) -> std::vector<Path>
{
    switch (sets.dummy_plugin_loading_mode)
    {
        case PluginLoadingMode::Limited: return plugins_loading_archive_limited(archive, sets);
        case PluginLoadingMode::Unlimited: return plugins_loading_archive_unlimited(archive, sets);
    }
    return {};
}

auto find_archive_name_using_plugins(std::span<const Path> plugins,
                                     const Settings &sets,
                                     ArchiveType type) -> std::optional<Path>
{
    if (plugins.empty())
        return std::nullopt;

    const auto archive_candidates = flux::from(plugins)
                                        .map([&sets, type](const auto &plugin) {
                                            return archive_loaded_by_plugin(plugin, sets, type);
                                        })
                                        .filter([](const auto &f) { return !exists(f); })
                                        // TODO: propose .first()
                                        .to<std::vector>();

    if (!archive_candidates.empty())
        return archive_candidates.front();

    // let's try to make a name based on the first plugin
    const auto plugin = plugins.front();
    const auto stem   = plugin.stem().u8string();

    for (uint8_t i = 1; i < std::numeric_limits<uint8_t>::max(); ++i)
    {
        const auto counter = common::as_utf8_string(std::to_string(i));
        const auto file    = plugin.parent_path()
                          / (stem + counter + archive_suffix_with_sep(sets, type) + sets.extension);
        if (!exists(file))
            return file;
    }

    return std::nullopt;
}

/// Returns an archive name that is unique, which is guaranteed by trying suitable suffixes
auto find_archive_name(const Path &directory,
                       const Settings &sets,
                       ArchiveType type) noexcept -> std::optional<Path>
{
    if (!is_directory(directory))
        return std::nullopt;

    auto plugins = list_plugins(directory, sets);

    if (plugins.empty())
        plugins.emplace_back(directory / (directory.filename().u8string() + sets.dummy_extension));

    if (auto name = find_archive_name_using_plugins(plugins, sets, type))
        return name;

    // alright, I have no idea how we can get here. But let's try to make a name just in case
    constexpr auto max_attempts = std::numeric_limits<uint8_t>::max();

    for (uint8_t i = 0; i < max_attempts; ++i)
    {
        auto filename = u8"archive - " + common::str_random(8);
        std::error_code error;
        auto file = directory / (filename + sets.extension);
        if (!exists(file, error) && !error)
            return file; // finally found a name, though I doubt the user will like it
    }

    // guess we are out of luck
    return std::nullopt;
}

void clean_dummy_plugins(std::span<const Path> plugins, const Settings &sets)
{
    if (!sets.dummy_plugin.has_value())
        return;

    const auto &dummy = *sets.dummy_plugin;

    auto is_dummy = [&dummy](const Path &f) {
        std::fstream file(f, std::ios::binary | std::ios::in | std::ios::ate);

        // It is safe to evaluate file size, as the embedded dummies are the smallest plugins possible
        return file && file.tellg() == static_cast<std::streamoff>(dummy.size());
    };

    flux::from(plugins).filter(is_dummy).for_each([](const auto &f) { fs::remove(f); });
}

void make_dummy_plugins(std::span<const Path> archives, const Settings &sets)
{
    if (!sets.dummy_plugin.has_value())
        return;

    for (const Path &arch : archives)
    {
        const auto associated_plugins = plugins_loading_archive(arch, sets);
        if (flux::any(associated_plugins, BTU_RESOLVE_OVERLOAD(btu::fs::exists)))
            continue;

        auto plugin = associated_plugins.front();
        plugin.replace_extension(sets.dummy_extension);

        const auto res = common::write_file_new(plugin, *sets.dummy_plugin);
        if (!res)
        {
            // TODO: what to do?
        }
    }
}

auto list_archive(const Path &dir, const Settings &sets) noexcept -> std::vector<Path>
{
    try
    {
        std::vector<Path> archives = flux::from_range(fs::directory_iterator(dir))
                                         .filter([&sets](const auto &f) {
                                             return f.is_regular_file()
                                                    && common::to_lower(f.path().extension().u8string())
                                                           == sets.extension;
                                         })
                                         .map([](const auto &f) { return f.path(); })
                                         .to<std::vector>();

        // When a single plugin can load multiple archives, it loads all archives such that their
        // name contains the name of the corresponding plugin as a prefix. Sorting the archive
        // names by length should ensure that only the required number of dummy plugins is created.
        flux::sort(archives, [](const auto p1, const auto p2) { return p1.stem() <= p2.stem(); });

        return archives;
    }
    catch (const std::exception &)
    {
        return {};
    }
}

void remake_dummy_plugins(const Path &directory, const Settings &sets)
{
    auto plugins = list_plugins(directory, sets);
    clean_dummy_plugins(plugins, sets);
    auto archives = list_archive(directory, sets);
    make_dummy_plugins(archives, sets);
}

} // namespace btu::bsa
