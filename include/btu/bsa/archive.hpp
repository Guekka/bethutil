#pragma once

#include "btu/bsa/detail/archive_type.hpp"
#include "btu/bsa/detail/common.hpp"

#include <bsa/bsa.hpp>

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

    auto unpack(const Path &out_path) -> void;

    [[nodiscard]] auto file_count() const noexcept -> size_t;

    template<typename VersionType>
    [[nodiscard]] auto get_version() const -> VersionType;

    [[nodiscard]] auto get_version() const noexcept -> ArchiveVersion;
    [[nodiscard]] auto get_archive() const noexcept -> const UnderlyingArchive &;

private:
    UnderlyingArchive archive_;
    std::mutex mutex_;

    ArchiveVersion version_{};
    bool compressed_{false};
};

} // namespace btu::bsa
