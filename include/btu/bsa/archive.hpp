#pragma once

#include "btu/common/metaprogramming.hpp"
#include "btu/common/path.hpp"

#include <bsa/bsa.hpp>
#include <nlohmann/json.hpp>

#include <variant>

namespace btu::bsa {
enum class Compression : std::uint8_t
{
    Yes,
    No,
};

template<class... Keys>
    requires requires(Keys... keys) { (std::string(keys.name()), ...); }
[[nodiscard]] auto virtual_to_local_path(const Keys &...a_keys) noexcept -> std::u8string
{
    std::u8string local;
    ((local += common::as_utf8(a_keys.name()), local += u8'/'), ...);
    local.pop_back();

    for (auto &c : local)
    {
        if (c == '\\' || c == '/')
        {
            c = Path::preferred_separator;
        }
    }
    common::make_valid(local, '_');
    return local;
}

namespace libbsa = ::bsa;

enum class ArchiveType : std::uint8_t
{
    Textures,
    Standard,
};

enum class ArchiveVersion : std::uint32_t
{
    tes3 = 1,
    tes4 = static_cast<std::underlying_type_t<ArchiveVersion>>(libbsa::tes4::version::tes4),
    fo3  = static_cast<std::underlying_type_t<ArchiveVersion>>(libbsa::tes4::version::fo3),
    tes5 = static_cast<std::underlying_type_t<ArchiveVersion>>(libbsa::tes4::version::tes5),
    sse  = static_cast<std::underlying_type_t<ArchiveVersion>>(libbsa::tes4::version::sse),
    fo4,
    starfield,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ArchiveType,
                             {{ArchiveType::Textures, "textures"}, {ArchiveType::Standard, "standard"}})

NLOHMANN_JSON_SERIALIZE_ENUM(ArchiveVersion,
                             {
                                 {ArchiveVersion::tes3, "tes3"},
                                 {ArchiveVersion::tes4, "tes4"},
                                 {ArchiveVersion::fo3, "fo3"},
                                 {ArchiveVersion::tes5, "tes5"},
                                 {ArchiveVersion::sse, "sse"},
                                 {ArchiveVersion::fo4, "fo4"},
                                 {ArchiveVersion::starfield, "starfield"},
                             })

using UnderlyingFile = std::variant<libbsa::tes3::file, libbsa::tes4::file, libbsa::fo4::file>;

class File final
{
public:
    explicit File(ArchiveVersion version, ArchiveType type);
    File(UnderlyingFile f, ArchiveVersion version, ArchiveType type);

    [[nodiscard]] auto compressed() const noexcept -> Compression;
    void compress();

    void read(Path path);
    void read(std::span<std::byte> src);

    void write(Path path) const;
    void write(binary_io::any_ostream &dst) const;

    [[nodiscard]] auto version() const noexcept -> ArchiveVersion;
    [[nodiscard]] auto type() const noexcept -> ArchiveType;
    [[nodiscard]] auto size() const noexcept -> size_t;

    template<typename T>
        requires btu::common::is_variant_member_v<T, UnderlyingFile>
    [[nodiscard]] auto as_raw_file() &&
    {
        return std::get<T>(std::move(file_));
    }

private:
    ArchiveVersion ver_;
    ArchiveType type_;
    UnderlyingFile file_;
};

class Archive final
{
    std::map<std::string, File> files_;

public:
    using value_type = decltype(files_)::value_type;

    Archive(ArchiveVersion ver, ArchiveType type);

    // While it is possible to copy an archive, it is better to disable implicit copying: it is a heavy operation
    Archive(const Archive &)                     = delete;
    auto operator=(const Archive &) -> Archive & = delete;

    Archive(Archive &&) noexcept                     = default;
    auto operator=(Archive &&) noexcept -> Archive & = default;

    ~Archive() = default;

    static auto read(Path path) -> std::optional<Archive>;
    [[nodiscard]] auto write(Path path) && -> bool;

    [[nodiscard]] auto emplace(std::string name, File file) -> bool;
    [[nodiscard]] auto get(const std::string &name) -> File &;

    [[nodiscard]] auto begin() noexcept { return files_.begin(); }
    [[nodiscard]] auto end() noexcept { return files_.end(); }

    [[nodiscard]] auto empty() const noexcept -> bool;

    [[nodiscard]] auto size() const noexcept -> size_t;

    [[nodiscard]] auto version() const noexcept -> ArchiveVersion { return ver_; }
    void set_version(ArchiveVersion version) noexcept;

    [[nodiscard]] auto type() const noexcept -> ArchiveType { return type_; }

    [[nodiscard]] auto file_size() const noexcept -> size_t;

private:
    Archive() = default;

    ArchiveVersion ver_;
    ArchiveType type_;
};

} // namespace btu::bsa
