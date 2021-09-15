#pragma once

#include "btu/bsa/detail/archive_type.hpp"

#include <bsa/bsa.hpp>

#include <functional>
#include <mutex>

namespace libbsa = ::bsa;

namespace btu::bsa {
using UnderlyingArchive = std::variant<libbsa::tes3::archive, libbsa::tes4::archive, libbsa::fo4::archive>;

namespace detail {
template<class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

[[nodiscard]] auto read_file(const std::filesystem::path &a_path) -> std::vector<std::byte>;

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
            c = std::filesystem::path::preferred_separator;
        }
    }

    return local;
}

[[nodiscard]] auto get_archive_identifier(const UnderlyingArchive &archive) -> std::string_view;

template<typename Version>
[[nodiscard]] auto archive_version(const UnderlyingArchive &archive, Version a_version) -> Version;
} // namespace detail

class Archive
{
public:
    Archive(const std::filesystem::path &a_path); // Read
    Archive(ArchiveVersion a_version, bool a_compressed);

    auto read(const std::filesystem::path &a_path) -> ArchiveVersion;
    void write(std::filesystem::path a_path);

    size_t add_file(const std::filesystem::path &a_root, const std::filesystem::path &a_path);
    size_t add_file(const std::filesystem::path &a_relative, std::vector<std::byte> a_data);

    using iteration_callback = std::function<void(const std::filesystem::path &, std::span<const std::byte>)>;
    void iterate_files(const iteration_callback &a_callback, bool skip_compressed = false);

    ArchiveVersion get_version() const noexcept;
    const UnderlyingArchive &get_archive() const noexcept;

private:
    UnderlyingArchive _archive;
    std::mutex _mutex;

    ArchiveVersion _version;
    bool _compressed;
};

} // namespace btu::bsa
