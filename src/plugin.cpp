/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/plugin.hpp"

#include "btu/common/algorithms.hpp"

#include <fstream>

namespace btu::bsa {
FilePath::FilePath(Path dir, OsString name, OsString suffix, Path ext, FileTypes type)
    : dir(std::move(dir))
    , name(std::move(name))
    , suffix(std::move(suffix))
    , ext(std::move(ext))
    , type(type)
{
}

auto FilePath::make(const Path &path, const Settings &sets, FileTypes type) -> std::optional<FilePath>
{
    if (fs::is_directory(path))
        return std::nullopt;

    FilePath file(path.parent_path(), path.stem().native(), {}, path.extension(), type);

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
    return (dir / full_name()).replace_extension(ext);
}

auto FilePath::full_name() const -> Path
{
    const auto count = Path(counter ? std::to_string(*counter) : "").native();
    const auto suf   = suffix.empty() ? BETHUTIL_BSA_STR("") : suffix_separator + suffix;
    return {name + count + suf};
}

auto FilePath::eat_digits(OsString &str) -> std::optional<int>
{
    size_t first_digit = str.length() - 1;
    for (; isdigit(str[first_digit]) != 0; --first_digit)
        ;
    ++first_digit;

    if (first_digit != str.length())
    {
        auto ret = std::stoi(str.substr(first_digit));
        str.erase(first_digit);
        return ret;
    }
    return std::nullopt;
}

auto FilePath::eat_suffix(OsString &str, const Settings &sets) -> OsString
{
    auto suffix_pos = str.rfind(suffix_separator);

    if (suffix_pos == OsString::npos)
        return {};

    auto suffix = str.substr(suffix_pos + suffix_separator.length());
    if (suffix != sets.suffix && suffix != sets.texture_suffix)
        return {};

    str.erase(suffix_pos);
    return suffix;
}

auto list_helper(const Path &folder_path, const Settings &sets, FileTypes type) -> std::vector<FilePath>
{
    std::vector<FilePath> res;
    for (const auto &f : fs::directory_iterator(folder_path))
        if (auto file = FilePath::make(f.path(), sets, type))
            res.emplace_back(*file);

    return res;
}

auto is_loaded(const FilePath &archive, const Settings &sets) -> bool
{
    return std::any_of(sets.plugin_extensions.cbegin(),
                       sets.plugin_extensions.cend(),
                       [&archive](const auto &ext) {
                           auto b            = archive;
                           b.ext             = ext;
                           bool const exact  = fs::exists(b.full_path());
                           b.suffix          = OsString{};
                           bool const approx = fs::exists(b.full_path());
                           return exact || approx;
                       });
}

auto list_plugins(const Path &folder_path, const Settings &sets) -> std::vector<FilePath>
{
    return list_helper(folder_path, sets, FileTypes::Plugin);
}

auto list_archive(const Path &folder_path, const Settings &sets) -> std::vector<FilePath>
{
    return list_helper(folder_path, sets, FileTypes::BSA);
}

auto find_archive_name(const Path &folder_path, const Settings &sets, ArchiveType type) -> FilePath
{
    std::vector<FilePath> plugins = list_plugins(folder_path, sets);

    if (plugins.empty())
        plugins.emplace_back(FilePath(folder_path, folder_path.filename(), {}, ".esp", FileTypes::Plugin));

    const Path suffix = [type, &sets] {
        if (type == ArchiveType::Textures)
        {
            return sets.texture_suffix.value();
        }
        return sets.suffix.value_or("");
    }();

    auto check_plugin = [&sets, &suffix](FilePath &file) {
        file.ext    = sets.extension;
        file.suffix = suffix;
        return !fs::exists(file.full_path());
    };

    for (auto &plugin : plugins)
        if (check_plugin(plugin))
            return plugin;

    FilePath plug                    = plugins.front();
    constexpr uint8_t max_iterations = UINT8_MAX;
    for (plug.counter = 0; plug.counter < max_iterations; ++(*plug.counter))
        if (check_plugin(plug))
            return plug;

    throw std::runtime_error("No btu/bsa/plugin name found after 256 tries.");
}

void clean_dummy_plugins(const Path &folder_path, const Settings &sets)
{
    if (!sets.s_dummy_plugin.has_value())
        return;
    const auto &dummy = *sets.s_dummy_plugin;

    for (const auto &plug : list_plugins(folder_path, sets))
    {
        const auto path = plug.full_path();
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::ate);

        // It is safe to evaluate file size, as the embedded dummies are the smallest plugins possible
        if (file && file.tellg() == dummy.size())
        {
            file.close();
            fs::remove(path);
        }
    }
}

void make_dummy_plugins(const Path &folder_path, const Settings &sets)
{
    if (!sets.s_dummy_plugin.has_value())
        return;

    for (auto &&bsa : list_archive(folder_path, sets))
    {
        if (is_loaded(bsa, sets))
            continue;

        bsa.ext    = sets.plugin_extensions.back();
        bsa.suffix = {};
        std::ofstream dummy(bsa.full_path(), std::ios::out | std::ios::binary);

        const auto dummy_bytes = *sets.s_dummy_plugin;
        dummy.write(reinterpret_cast<const char *>(dummy_bytes.data()), dummy_bytes.size()); // NOLINT
    }
}

} // namespace btu::bsa
