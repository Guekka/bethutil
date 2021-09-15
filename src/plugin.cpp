/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/plugin.hpp"

#include "btu/common/algorithms.hpp"

#include <fstream>

namespace btu::bsa {
FilePath::FilePath(Path dir, OsString name, OsString suffix, Path ext, FileTypes type)
    : dir_(std::move(dir))
    , name_(std::move(name))
    , suffix_(std::move(suffix))
    , ext_(std::move(ext))
    , type_(type)
{
}

std::optional<FilePath> FilePath::make(const Path &path, const Settings &sets, FileTypes type)
{
    if (fs::is_directory(path))
        return std::nullopt;

    FilePath file(path.parent_path(), path.stem().native(), {}, path.extension(), type);

    if (type == FileTypes::Plugin && !common::contains(sets.pluginExtensions, file.ext_))
        return std::nullopt;

    if (type == FileTypes::BSA && file.ext_ != sets.extension)
        return std::nullopt;

    file.counter_ = eat_digits(file.name_);
    file.suffix_  = eat_suffix(file.name_, sets);

    if (!file.counter_.has_value())
        file.counter_ = eat_digits(file.name_);

    return file;
}

fs::path FilePath::full_path() const
{
    return (dir_ / full_name()).replace_extension(ext_);
}

btu::bsa::fs::path FilePath::full_name() const
{
    const auto counter = Path(counter_ ? std::to_string(*counter_) : "").native();
    const auto suffix  = suffix_.empty() ? BETHUTIL_BSA_STR("") : suffixSeparator + suffix_;
    return Path(name_ + counter + suffix);
}

std::optional<int> FilePath::eat_digits(OsString &str)
{
    size_t firstDigit = str.length() - 1;
    for (; isdigit(str[firstDigit]); --firstDigit)
        ;
    ++firstDigit;

    if (firstDigit != str.length())
    {
        auto ret = std::stoi(str.substr(firstDigit));
        str.erase(firstDigit);
        return ret;
    }
    return std::nullopt;
}

OsString FilePath::eat_suffix(OsString &str, const Settings &sets)
{
    auto suffixPos = str.rfind(suffixSeparator);

    if (suffixPos == OsString::npos)
        return {};

    auto suffix = str.substr(suffixPos + suffixSeparator.length());
    if (suffix != sets.suffix && suffix != sets.textureSuffix)
        return {};

    str.erase(suffixPos);
    return suffix;
}

std::vector<FilePath> list_helper(const Path &folderPath, const Settings &sets, FileTypes type)
{
    std::vector<FilePath> res;
    for (const auto &f : fs::directory_iterator(folderPath))
        if (auto file = FilePath::make(f.path(), sets, type))
            res.emplace_back(*file);

    return res;
}

bool is_loaded(const FilePath &archive, const Settings &sets)
{
    return std::any_of(sets.pluginExtensions.cbegin(),
                       sets.pluginExtensions.cend(),
                       [&archive](const auto &ext) {
                           auto b            = archive;
                           b.ext_            = ext;
                           bool const exact  = fs::exists(b.full_path());
                           b.suffix_         = OsString{};
                           bool const approx = fs::exists(b.full_path());
                           return exact || approx;
                       });
}

std::vector<FilePath> list_plugins(const Path &folderPath, const Settings &sets)
{
    return list_helper(folderPath, sets, FileTypes::Plugin);
}

std::vector<FilePath> list_archive(const Path &folderPath, const Settings &sets)
{
    return list_helper(folderPath, sets, FileTypes::BSA);
}

FilePath find_archive_name(const Path &folderPath, const Settings &sets, ArchiveType type)
{
    std::vector<FilePath> plugins = list_plugins(folderPath, sets);

    if (plugins.empty())
        plugins.emplace_back(FilePath(folderPath, folderPath.filename(), {}, ".esp", FileTypes::Plugin));

    const Path suffix = [type, &sets] {
        if (type == ArchiveType::Textures)
            return sets.textureSuffix.value();
        return sets.suffix.value_or("");
    }();

    auto checkPlugin = [&sets, &suffix](FilePath &file) {
        file.ext_    = sets.extension;
        file.suffix_ = suffix;
        return !fs::exists(file.full_path());
    };

    for (auto &plugin : plugins)
        if (checkPlugin(plugin))
            return plugin;

    FilePath plug                   = plugins.front();
    constexpr uint8_t maxIterations = UINT8_MAX;
    for (plug.counter_ = 0; plug.counter_ < maxIterations; ++(*plug.counter_))
        if (checkPlugin(plug))
            return plug;

    throw std::runtime_error("No btu/bsa/plugin name found after 256 tries.");
}

void clean_dummy_plugins(const btu::bsa::fs::path &folderPath, const Settings &sets)
{
    if (!sets.sDummyPlugin.has_value())
        return;
    const auto &dummy = *sets.sDummyPlugin;

    for (const auto &plug : list_plugins(folderPath, sets))
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

void make_dummy_plugins(const fs::path &folderPath, const Settings &sets)
{
    if (!sets.sDummyPlugin.has_value())
        return;

    for (auto &&bsa : list_archive(folderPath, sets))
    {
        if (is_loaded(bsa, sets))
            continue;

        bsa.ext_    = sets.pluginExtensions.back();
        bsa.suffix_ = {};
        std::ofstream dummy(bsa.full_path(), std::ios::out | std::ios::binary);

        const auto dummyBytes = *sets.sDummyPlugin;
        dummy.write(reinterpret_cast<const char *>(dummyBytes.data()), dummyBytes.size());
    }
}

} // namespace btu::bsa
