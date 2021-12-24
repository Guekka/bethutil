/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/bsa/archive.hpp"
#include "btu/common/path.hpp"

#include <boost/stl_interfaces/iterator_interface.hpp>

#include <variant>

namespace btu::bsa {
class Archive;
}

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

    [[nodiscard]] auto begin() -> Iterator;
    [[nodiscard]] auto end() -> Iterator;

private:
    std::vector<Path> archives_;
    std::vector<Path> files_;

    size_t count_;

    Path dir_;
    std::u8string archive_ext_;
};

class ModFolder::Iterator
    : public boost::stl_interfaces::iterator_interface<Iterator,
                                                       std::forward_iterator_tag,
                                                       ModFile,
                                                       ModFile,
                                                       boost::stl_interfaces::proxy_arrow_result<ModFile>>
{
    using base_type
        = boost::stl_interfaces::iterator_interface<Iterator,
                                                    std::forward_iterator_tag,
                                                    ModFile,
                                                    ModFile,
                                                    boost::stl_interfaces::proxy_arrow_result<ModFile>>;

    Iterator() noexcept {}

    Iterator(ModFolder &mf) noexcept
        : is_archive(true)
        , mf_(&mf)
        , files_(mf.archives_)
    {
        if (files_.empty())
        {
            files_     = mf_->files_;
            is_archive = false;
        }

        if (is_archive)
            load_archive();
        else
            file_ = ModFile(detail::ModFileDisk{files_.front()});
    }

    auto operator*() const noexcept -> const ModFile & { return file_; }
    Iterator &operator++() noexcept
    {
        files_ = files_.subspan(1);
        if (files_.empty() && is_archive)
            file_ = ModFile(detail::ModFileDisk{files_.front()});
        else if (is_archive)
            load_archive();
        else
            file_ = ModFile(detail::ModFileDisk{files_.front()});
        return *this;
    }
    auto operator==(const ModFolder::Iterator &other) const noexcept -> bool
    {
        return mf_ == other.mf_ && files_.data() == other.files_.data() && arch_ == other.arch_;
    }

    using base_type::operator++;

private:
    void load_archive()
    {
        arch_ = std::make_unique<btu::bsa::Archive>(files_.front());
        file_ = ModFile(arch_->begin());
    }

    std::unique_ptr<btu::bsa::Archive> arch_;
    bool is_archive;

    ModFolder *mf_{};
    std::span<Path> files_{};

    ModFile file_{};
};

//BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT(Iterator, std::forward_iterator);
} // namespace btu::modmanager
