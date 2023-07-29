#pragma once

#include "btu/bsa/detail/archive_type.hpp"
#include "btu/common/metaprogramming.hpp"
#include "btu/common/path.hpp"

#include <bsa/bsa.hpp>

#include <functional>
#include <variant>

namespace btu::bsa {
enum class Compression
{
    Yes,
    No,
};

template<class... Keys>
requires requires(Keys... keys)
{
    (std::string(keys.name()), ...);
}
[[nodiscard]] auto virtual_to_local_path(Keys &&...a_keys) noexcept -> std::u8string
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

namespace libbsa     = ::bsa;
using UnderlyingFile = std::variant<libbsa::tes3::file, libbsa::tes4::file, libbsa::fo4::file>;

class File final
{
public:
    explicit File(ArchiveVersion v);
    File(UnderlyingFile f, ArchiveVersion v);

    [[nodiscard]] auto compressed() const noexcept -> Compression;
    void decompress();
    void compress();

    void read(Path path);
    void read(std::span<std::byte> src);

    void write(Path path) const;
    void write(binary_io::any_ostream &dst) const;

    [[nodiscard]] auto version() const noexcept -> ArchiveVersion;
    [[nodiscard]] auto size() const noexcept -> size_t;

    template<typename T>
    requires btu::common::is_variant_member_v<T, UnderlyingFile>
    [[nodiscard]] auto as_raw_file() && { return std::get<T>(std::move(file_)); }

private:
    UnderlyingFile file_;
    ArchiveVersion ver_;
};

using Archive = std::map<std::string, File, std::less<>>;

auto read_archive(Path path) -> std::optional<Archive>;
void write_archive(Archive &&arch, Path path);

[[nodiscard]] auto archive_version(const Archive &arch) noexcept -> std::optional<ArchiveVersion>;
void set_archive_version(Archive &arch, ArchiveVersion version) noexcept;

[[nodiscard]] auto archive_type(const Archive &arch) noexcept -> std::optional<ArchiveType>;
} // namespace btu::bsa
