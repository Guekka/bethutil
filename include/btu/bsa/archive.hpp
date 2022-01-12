#pragma once

#include "btu/bsa/detail/archive_type.hpp"
#include "btu/bsa/detail/common.hpp"

#include <bsa/bsa.hpp>
#include <neo/iterator_facade.hpp>

#include <functional>
#include <mutex>
#include <variant>

namespace libbsa = ::bsa;

namespace btu::bsa {
using UnderlyingArchive = std::variant<libbsa::tes3::archive, libbsa::tes4::archive, libbsa::fo4::archive>;
using UnderlyingFile    = std::variant<libbsa::tes3::file, libbsa::tes4::file, libbsa::fo4::file>;

template<class... Keys>
[[nodiscard]] auto virtual_to_local_path(Keys &&...a_keys) -> std::u8string
{
    std::u8string local;
    ((local += btu::common::as_utf8(a_keys.name()), local += u8'/'), ...);
    local.pop_back();

    for (auto &c : local)
    {
        if (c == '\\' || c == '/')
        {
            c = Path::preferred_separator;
        }
    }
    btu::common::make_valid(local, '_');
    return local;
}

[[nodiscard]] auto get_archive_identifier(const UnderlyingArchive &archive) -> std::string_view;

template<typename Version>
[[nodiscard]] auto archive_version(const UnderlyingArchive &archive, Version a_version) -> Version;

class Archive final
{
public:
    explicit Archive(const Path &a_path); // Read
    Archive(ArchiveVersion a_version, bool a_compressed);

    auto read(Path a_path) -> ArchiveVersion;
    auto write(Path a_path) -> void;

    auto add_file(const Path &a_relative, UnderlyingFile file) -> void;
    auto add_file(const Path &a_root, const Path &a_path) -> void;
    auto add_file(const Path &a_relative, std::vector<std::byte> a_data) -> void;

    auto write_file(const UnderlyingFile &file, const Path &path) -> void;
    auto unpack(const Path &out_path) -> void;
    auto unpack_file(const Path &rel_path, const Path &out_path) -> void;

    auto file_count() const noexcept -> size_t;

    class Iterator;

    auto begin() -> Iterator;
    auto end() -> Iterator;

    [[nodiscard]] auto get_version() const noexcept -> ArchiveVersion;
    [[nodiscard]] auto get_archive() const noexcept -> const UnderlyingArchive &;

private:
    UnderlyingArchive archive_;
    std::mutex mutex_;

    ArchiveVersion version_{};
    bool compressed_{false};
};

namespace detail {
class WriteableFile
{
private:
    std::string name_;
};

class Tes4Iter : public neo::iterator_facade<Tes4Iter>
{
public:
    using base_type = neo::iterator_facade<Tes4Iter>;

    Tes4Iter() noexcept = default;
    Tes4Iter(libbsa::tes4::archive &arch) noexcept;

    static auto end(libbsa::tes4::archive &arch) -> Tes4Iter;

    auto dereference() const noexcept -> std::string;
    auto increment() noexcept -> Tes4Iter &;

    auto write(binary_io::any_ostream &os) const -> void;

    auto operator==(Tes4Iter other) const noexcept -> bool;

private:
    libbsa::tes4::version ver_;

    libbsa::tes4::archive::iterator dir_;
    libbsa::tes4::archive::iterator dir_end_;
    libbsa::tes4::archive::mapped_type::iterator file_;
};

static_assert(std::forward_iterator<Tes4Iter>);
} // namespace detail

using UnderlyingIterator
    = std::variant<libbsa::tes3::archive::iterator, detail::Tes4Iter, libbsa::fo4::archive::iterator>;

class Archive::Iterator : public neo::iterator_facade<Archive::Iterator>
{
public:
    using base_type = neo::iterator_facade<Archive::Iterator>;

    Iterator() noexcept {}

    Iterator(UnderlyingIterator it, libbsa::fo4::format fo4_ver = libbsa::fo4::format::general) noexcept;

    auto dereference() const noexcept -> std::string;
    auto increment() noexcept -> Archive::Iterator &;

    auto write(binary_io::any_ostream &os) const -> void;

    auto operator==(Iterator other) const noexcept -> bool;

private:
    UnderlyingIterator it_;
    libbsa::fo4::format fo4_ver_;
};
static_assert(std::forward_iterator<Archive::Iterator>);

} // namespace btu::bsa
