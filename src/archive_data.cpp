/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/archive_data.hpp"

#include "btu/bsa/settings.hpp"
#include "btu/common/algorithms.hpp"

namespace btu::bsa {
ArchiveData::ArchiveData(const Settings &sets, ArchiveType type)
    : max_size_(sets.maxSize)
    , type_(type)
    , version_(sets.format)
{
}

ArchiveData::Size &ArchiveData::Size::operator+=(const Size &other)
{
    compressed += other.compressed;
    uncompressed += other.uncompressed;
    return *this;
}

Path ArchiveData::find_name(const Path &folder, const Settings &sets) const
{
    return find_archive_name(folder, sets, type_).full_path();
}

bool ArchiveData::empty() const
{
    return size().uncompressed == 0;
}

bool ArchiveData::add_file(Path path, std::optional<Size> override)
{
    const auto fsize = get_file_size(path, override);
    if (size_.compressed + fsize.compressed > max_size_)
        return false;

    files_.emplace_back(std::move(path));
    size_ += fsize;

    return true;
}

ArchiveData &ArchiveData::operator+=(const ArchiveData &other)
{
    if (other.version_ != version_)
        throw std::runtime_error("Cannot merge ArchiveData of different format");

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

ArchiveData ArchiveData::operator+(ArchiveData const &other) const
{
    ArchiveData copy = *this;
    copy += other;
    return copy;
}

ArchiveData::Size ArchiveData::get_file_size(const common::Path &path, std::optional<Size> override) const
{
    if (override.has_value())
        return override.value();

    Size ret;

    // Incompressible archives have the same compressed and uncompressed size
    if (k_acurate_size_estimation && type() != ArchiveType::Incompressible)
    {
        auto data        = detail::read_file(path);
        ret.uncompressed = data.size();
        ret.compressed   = [this, data = std::move(data)] {
            switch (version_)
            {
                case ArchiveVersion::tes3: return data.size();
                case ArchiveVersion::fo3:
                case ArchiveVersion::tes4:
                case ArchiveVersion::sse:
                {
                    libbsa::tes4::file f;
                    f.set_data(std::move(data));
                    return f.compress_bound(static_cast<libbsa::tes4::version>(version_));
                }

                case ArchiveVersion::fo4:
                case ArchiveVersion::fo4dx:
                {
                    libbsa::fo4::chunk f;
                    f.set_data(std::move(data));
                    return f.compress_bound();
                }
                default: return data.size();
            }
        }();
    }
    else
    {
        ret.compressed = ret.uncompressed = fs::file_size(path);
    }

    return ret;
}

} // namespace btu::bsa
