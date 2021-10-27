#include "btu/bsa/detail/backends/rsm_archive.hpp"

#include "btu/bsa/detail/common.hpp"
#include "btu/common/filesystem.hpp"

#include <execution>
#include <fstream>

namespace btu::bsa::detail {

// Defined in fo4 part
auto pack_fo4dx_file(std::span<std::byte> data, bool compress) -> libbsa::fo4::file;
auto unpack_fo4dx_file(libbsa::fo4::file &file) -> std::vector<std::byte>;

[[nodiscard]] auto get_archive_identifier(const UnderlyingArchive &archive) -> std::string_view
{
    const auto visiter = btu::common::overload{
        [](const libbsa::tes3::archive &) { return "tes3"; },
        [](const libbsa::tes4::archive &) { return "tes4"; },
        [](const libbsa::fo4::archive &) { return "fo4"; },
    };
    return std::visit(visiter, archive);
}

template<typename VersionType>
[[nodiscard]] auto archive_version(const UnderlyingArchive &archive, ArchiveVersion a_version) -> VersionType
{
    const bool correct = [&] {
        switch (a_version)
        {
            case ArchiveVersion::tes3:
            {
                const bool same = std::same_as<VersionType, std::uint32_t>;
                return same && std::holds_alternative<libbsa::tes3::archive>(archive);
            }
            case ArchiveVersion::tes4:
            case ArchiveVersion::fo3:
            case ArchiveVersion::sse:
            {
                const bool same = std::same_as<VersionType, libbsa::tes4::version>;
                return same && std::holds_alternative<libbsa::tes4::archive>(archive);
            }
            case ArchiveVersion::fo4:
            case ArchiveVersion::fo4dx:
            {
                const bool same = std::same_as<VersionType, libbsa::fo4::format>;
                return same && std::holds_alternative<libbsa::fo4::archive>(archive);
            }
            default: return false;
        }
    }();

    if (!correct)
    {
        throw std::runtime_error("Mismatch between requested version and variant type");
    }

    return static_cast<VersionType>(libbsa::detail::to_underlying(a_version));
}

template auto archive_version<std::uint32_t>(const UnderlyingArchive &, ArchiveVersion) -> std::uint32_t;
template auto archive_version<libbsa::tes4::version>(const UnderlyingArchive &, ArchiveVersion)
    -> libbsa::tes4::version;
template auto archive_version<libbsa::fo4::format>(const UnderlyingArchive &, ArchiveVersion)
    -> libbsa::fo4::format;

RsmArchive::RsmArchive(const Path &a_path)
{
    read(a_path);
}

RsmArchive::RsmArchive(ArchiveVersion a_version, bool a_compressed)
    : version_(a_version)
    , compressed_(a_compressed)
{
    switch (version_)
    {
        case ArchiveVersion::tes3: archive_ = libbsa::tes3::archive{}; break;
        case ArchiveVersion::tes4:
        case ArchiveVersion::fo3:
        case ArchiveVersion::sse:
        {
            libbsa::tes4::archive bsa;
            auto flags = libbsa::tes4::archive_flag::directory_strings
                         | libbsa::tes4::archive_flag::file_strings;
            if (compressed_)
            {
                flags |= libbsa::tes4::archive_flag::compressed;
            }
            bsa.archive_flags(flags);
            archive_ = std::move(bsa);
            break;
        }
        case ArchiveVersion::fo4:
        case ArchiveVersion::fo4dx: archive_ = libbsa::fo4::archive{};
    }
}

auto RsmArchive::read(Path a_path) -> ArchiveVersion
{
    const auto format = libbsa::guess_file_format(a_path).value();

    const auto read = [this, &a_path](auto archive) {
        auto format = archive.read(std::move(a_path));
        archive_    = std::move(archive);
        return static_cast<ArchiveVersion>(format);
    };

    version_ = [&] {
        switch (format)
        {
            case libbsa::file_format::fo4: return read(libbsa::fo4::archive{});
            case libbsa::file_format::tes3:
            {
                libbsa::tes3::archive archive;
                archive.read(std::move(a_path));
                archive_ = std::move(archive);
                return ArchiveVersion::tes3;
            }
            case libbsa::file_format::tes4: return read(libbsa::tes4::archive{});
            default: libbsa::detail::declare_unreachable();
        }
    }();
    return version_;
}

auto RsmArchive::write(Path a_path) -> void
{
    const auto writer = btu::common::overload{
        [&](libbsa::tes3::archive &bsa) { bsa.write(a_path); },
        [&](libbsa::tes4::archive &bsa) {
            const auto version = detail::archive_version<libbsa::tes4::version>(archive_, version_);
            bsa.write(a_path, version);
        },
        [&](libbsa::fo4::archive &ba2) {
            const auto version = detail::archive_version<libbsa::fo4::format>(archive_, version_);
            ba2.write(a_path, version);
        },
    };

    std::visit(writer, archive_);
}

auto RsmArchive::add_file(const Path &a_root, const Path &a_path) -> void
{
    const auto relative = a_path.lexically_relative(a_root).lexically_normal();
    const auto data     = btu::common::read_file(a_path);
    return add_file(relative, data);
}

auto RsmArchive::add_file(const Path &a_relative, std::vector<std::byte> a_data) -> void
{
    const auto adder = btu::common::overload{
        [&](libbsa::tes3::archive &bsa) {
            libbsa::tes3::file f;

            f.set_data(std::move(a_data));

            std::scoped_lock lock(mutex_);

            bsa.insert(a_relative.lexically_normal().generic_string(), std::move(f));
        },
        [&, this](libbsa::tes4::archive &bsa) {
            libbsa::tes4::file f;
            const auto version = detail::archive_version<libbsa::tes4::version>(archive_, version_);
            f.set_data(std::move(a_data));

            if (compressed_)
                f.compress(version);

            std::scoped_lock lock(mutex_);
            const auto d = [&]() {
                const auto key = a_relative.parent_path().lexically_normal().generic_string();
                if (bsa.find(key) == bsa.end())
                {
                    bsa.insert(key, libbsa::tes4::directory{});
                }
                return bsa[key];
            }();

            d->insert(a_relative.filename().lexically_normal().generic_string(), std::move(f));
        },
        [&, this](libbsa::fo4::archive &ba2) {
            const auto version = detail::archive_version<libbsa::fo4::format>(archive_, version_);

            auto file = [&] {
                if (version == libbsa::fo4::format::general)
                {
                    libbsa::fo4::file f;
                    auto &chunk = f.emplace_back();
                    chunk.set_data(std::move(a_data));

                    if (compressed_)
                        chunk.compress();
                    return f;
                }
                return pack_fo4dx_file(a_data, compressed_);
            }();

            std::scoped_lock lock(mutex_);

            ba2.insert(a_relative.lexically_normal().generic_string(), std::move(file));
        },
    };

    std::visit(adder, archive_);
}

auto RsmArchive::iterate_files(const iteration_callback &a_callback, bool skip_compressed) -> void
{
    auto visiter = btu::common::overload{
        [&](libbsa::tes3::archive &bsa) {
            std::for_each(std::execution::par, bsa.cbegin(), bsa.cend(), [&](auto &&pair) {
                const auto [key, file] = pair;
                const auto relative    = detail::virtual_to_local_path(key);
                const auto bytes       = file.as_bytes();
                a_callback(relative, bytes);
            });
        },
        [&](libbsa::tes4::archive &bsa) {
            for (auto &dir : bsa)
            {
                std::for_each(std::execution::par, dir.second.begin(), dir.second.end(), [&](auto &&file) {
                    const auto relative = detail::virtual_to_local_path(dir.first, file.first);
                    const auto ver      = detail::archive_version<libbsa::tes4::version>(archive_, version_);

                    if (file.second.compressed())
                    {
                        if (skip_compressed)
                        {
                            return;
                        }
                        file.second.decompress(ver);
                    }
                    a_callback(relative, file.second.as_bytes());
                });
            }
        },
        [&](libbsa::fo4::archive &ba2) {
            std::for_each(std::execution::par, ba2.begin(), ba2.end(), [&](auto &&pair) {
                auto &&[key, file]  = pair;
                const auto relative = detail::virtual_to_local_path(key);

                if (version_ == ArchiveVersion::fo4)
                {
                    // Fast path, one chunk
                    if (file.size() == 1)
                    {
                        a_callback(relative, file[0].as_bytes());
                    }
                    else
                    {
                        std::vector<std::byte> bytes;
                        for (auto &chunk : file)
                        {
                            if (chunk.compressed())
                            {
                                if (skip_compressed)
                                {
                                    return;
                                }
                                chunk.decompress();
                            }

                            const auto chunk_bytes = chunk.as_bytes();
                            bytes.reserve(bytes.size() + chunk_bytes.size());
                            bytes.insert(bytes.end(), chunk_bytes.begin(), chunk_bytes.end());
                        }
                        a_callback(relative, bytes);
                    }
                }
                else
                {
                    const auto data = unpack_fo4dx_file(file);
                    a_callback(relative, data);
                }
            });
        },
    };

    std::visit(visiter, archive_);
}

auto RsmArchive::get_version() const noexcept -> ArchiveVersion
{
    return version_;
}

auto RsmArchive::get_archive() const noexcept -> const UnderlyingArchive &
{
    return archive_;
}
} // namespace btu::bsa::detail
