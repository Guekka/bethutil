#pragma once

#include "btu/bsa/detail/backends/archive.hpp"

#include <bsa/bsa.hpp>

#include <functional>
#include <mutex>
#include <variant>

namespace libbsa = ::bsa;

namespace btu::bsa::detail {
using UnderlyingArchive = std::variant<libbsa::tes3::archive, libbsa::tes4::archive, libbsa::fo4::archive>;

template<class... Keys>
[[nodiscard]] auto virtual_to_local_path(Keys &&...a_keys) -> std::string
{
    std::string local;
    ((local += a_keys.name(), local += '/'), ...);
    local.pop_back();

    for (auto &c : local)
    {
        if (c == '\\' || c == '/')
        {
            c = Path::preferred_separator;
        }
    }

    return local;
}

[[nodiscard]] auto get_archive_identifier(const UnderlyingArchive &archive) -> std::string_view;

template<typename Version>
[[nodiscard]] auto archive_version(const UnderlyingArchive &archive, Version a_version) -> Version;

class RsmArchive : public Archive
{
public:
    explicit RsmArchive(const Path &a_path); // Read
    RsmArchive(ArchiveVersion a_version, bool a_compressed);

    auto read(const Path &a_path) -> ArchiveVersion override;
    auto write(Path a_path) -> void override;

    auto add_file(const Path &a_root, const Path &a_path) -> void override;
    auto add_file(const Path &a_relative, std::vector<std::byte> a_data) -> void override;

    using iteration_callback = std::function<void(const Path &, std::span<const std::byte>)>;
    auto iterate_files(const iteration_callback &a_callback, bool skip_compressed = false) -> void override;

    [[nodiscard]] auto get_version() const noexcept -> ArchiveVersion override;
    [[nodiscard]] auto get_archive() const noexcept -> const UnderlyingArchive &;

private:
    UnderlyingArchive archive_;
    std::mutex mutex_;

    ArchiveVersion version_;
    bool compressed_;
};

} // namespace btu::bsa::detail
