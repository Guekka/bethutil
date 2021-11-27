/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/archive_data.hpp"

#include "btu/bsa/settings.hpp"
#include "btu/common/algorithms.hpp"

namespace btu::bsa {
ArchiveData::ArchiveData(const Settings &sets, ArchiveType type)
    : max_size_(sets.max_size)
    , type_(type)
    , version_(type == ArchiveType::Textures ? sets.texture_format.value_or(sets.format) : sets.format)
{
}

auto ArchiveData::Size::operator+=(const Size &other) -> ArchiveData::Size &
{
    compressed += other.compressed;
    uncompressed += other.uncompressed;
    return *this;
}

auto ArchiveData::find_name(const Path &folder, const Settings &sets) const -> Path
{
    return find_archive_name(folder, sets, type_).full_path();
}

auto ArchiveData::empty() const -> bool
{
    return size().uncompressed == 0;
}

auto ArchiveData::add_file(Path path, std::optional<Size> override) -> bool
{
    const auto fsize = get_file_size(path, override);
    if (size_.compressed + fsize.compressed > max_size_)
        return false;

    files_.emplace_back(std::move(path));
    size_ += fsize;

    return true;
}

auto ArchiveData::operator+=(const ArchiveData &other) -> ArchiveData &
{
    if (size().compressed + other.size().compressed > max_size_)
        throw std::runtime_error("Cannot merge ArchiveData with file size over max size");

    size_ += other.size_;

    files_.insert(files_.end(), other.files_.begin(), other.files_.end());

    if (type_ == ArchiveType::Incompressible || other.type_ == ArchiveType::Incompressible)
        type_ = ArchiveType::Incompressible;
    else if (type_ != other.type_)
        type_ = ArchiveType::Standard;
    return *this;
}

auto ArchiveData::operator+(ArchiveData const &other) const -> ArchiveData
{
    ArchiveData copy = *this;
    copy += other;
    return copy;
}

auto ArchiveData::get_file_size(const common::Path &path, std::optional<Size> override) -> ArchiveData::Size
{
    if (override.has_value())
        return override.value();

    Size ret{};
    ret.compressed = ret.uncompressed = fs::file_size(path);
    return ret;
}

} // namespace btu::bsa
