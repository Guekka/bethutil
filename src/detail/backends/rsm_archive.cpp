#include "btu/bsa/detail/backends/rsm_archive.hpp"

#include "btu/bsa/detail/common.hpp"

#include <execution>
#include <fstream>

namespace btu::bsa::detail {

[[nodiscard]] auto get_archive_identifier(const UnderlyingArchive &archive) -> std::string_view
{
    const auto visiter = detail::overload{
        [](libbsa::tes3::archive) { return "tes3"; },
        [](libbsa::tes4::archive) { return "tes4"; },
        [](libbsa::fo4::archive) { return "fo4"; },
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

template std::uint32_t archive_version<std::uint32_t>(const UnderlyingArchive &, ArchiveVersion);
template libbsa::tes4::version archive_version<libbsa::tes4::version>(const UnderlyingArchive &,
                                                                      ArchiveVersion);
template libbsa::fo4::format archive_version<libbsa::fo4::format>(const UnderlyingArchive &, ArchiveVersion);

RsmArchive::RsmArchive(const std::filesystem::path &a_path)
{
    read(a_path);
}

RsmArchive::RsmArchive(ArchiveVersion a_version, bool a_compressed)
    : _version(a_version)
    , _compressed(a_compressed)
{
    switch (_version)
    {
        case ArchiveVersion::tes3: _archive = libbsa::tes3::archive{}; break;
        case ArchiveVersion::tes4:
        case ArchiveVersion::fo3:
        case ArchiveVersion::sse:
        {
            libbsa::tes4::archive bsa;
            auto flags = libbsa::tes4::archive_flag::directory_strings
                         | libbsa::tes4::archive_flag::file_strings;
            if (_compressed)
            {
                flags |= libbsa::tes4::archive_flag::compressed;
            }
            bsa.archive_flags(flags);
            _archive = std::move(bsa);
            break;
        }
        case ArchiveVersion::fo4:
        case ArchiveVersion::fo4dx: _archive = libbsa::fo4::archive{};
    }
}

auto RsmArchive::read(const std::filesystem::path &a_path) -> ArchiveVersion
{
    const auto format = libbsa::guess_file_format(a_path).value();

    const auto read = [this, &a_path](auto archive) {
        auto format = archive.read(std::move(a_path));
        _archive    = std::move(archive);
        return static_cast<ArchiveVersion>(format);
    };

    _version = [&] {
        switch (format)
        {
            case libbsa::file_format::fo4: return read(libbsa::fo4::archive{});
            case libbsa::file_format::tes3:
            {
                libbsa::tes3::archive archive;
                archive.read(std::move(a_path));
                _archive = std::move(archive);
                return ArchiveVersion::tes3;
            }
            case libbsa::file_format::tes4: return read(libbsa::tes4::archive{});
            default: libbsa::detail::declare_unreachable();
        }
    }();
    if (_version == ArchiveVersion::fo4dx)
    {
        throw std::runtime_error("unsupported fo4 archive format");
    }
    return _version;
}

void RsmArchive::write(std::filesystem::path a_path)
{
    const auto writer = detail::overload{
        [&](libbsa::tes3::archive &bsa) { bsa.write(a_path); },
        [&](libbsa::tes4::archive &bsa) {
            const auto version = detail::archive_version<libbsa::tes4::version>(_archive, _version);
            bsa.write(a_path, version);
        },
        [&](libbsa::fo4::archive &ba2) {
            const auto version = detail::archive_version<libbsa::fo4::format>(_archive, _version);
            ba2.write(a_path, version);
        },
    };

    std::visit(writer, _archive);
}

size_t RsmArchive::add_file(const std::filesystem::path &a_root, const std::filesystem::path &a_path)
{
    const auto relative = a_path.lexically_relative(a_root).lexically_normal();
    const auto data     = detail::read_file(a_path);
    return add_file(relative, data);
}

size_t RsmArchive::add_file(const std::filesystem::path &a_relative, std::vector<std::byte> a_data)
{
    const auto adder = detail::overload{
        [&](libbsa::tes3::archive &bsa) {
            libbsa::tes3::file f;

            auto ret = a_data.size();
            f.set_data(std::move(a_data));

            std::scoped_lock lock(_mutex);

            bsa.insert(a_relative.lexically_normal().generic_string(), std::move(f));
            return ret;
        },
        [&, this](libbsa::tes4::archive &bsa) {
            libbsa::tes4::file f;
            const auto version = detail::archive_version<libbsa::tes4::version>(_archive, _version);
            f.set_data(std::move(a_data));

            if (_compressed)
                f.compress(version);

            std::scoped_lock lock(_mutex);
            const auto d = [&]() {
                const auto key = a_relative.parent_path().lexically_normal().generic_string();
                if (bsa.find(key) == bsa.end())
                {
                    bsa.insert(key, libbsa::tes4::directory{});
                }
                return bsa[key];
            }();

            auto ret = f.size();
            d->insert(a_relative.filename().lexically_normal().generic_string(), std::move(f));
            return ret;
        },
        [&, this](libbsa::fo4::archive &ba2) {
            assert(detail::archive_version<libbsa::fo4::format>(_archive, _version)
                       == libbsa::fo4::format::general
                   && "directx ba2 not supported");
            libbsa::fo4::file f;
            auto &chunk = f.emplace_back();
            chunk.set_data(std::move(a_data));

            if (_compressed)
                chunk.compress();

            std::scoped_lock lock(_mutex);

            auto ret = chunk.size();
            ba2.insert(a_relative.lexically_normal().generic_string(), std::move(f));
            return ret;
        },
    };

    return std::visit(adder, _archive);
}

void RsmArchive::iterate_files(const iteration_callback &a_callback, bool skip_compressed)
{
    auto visiter = detail::overload{
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
                    const auto ver      = detail::archive_version<libbsa::tes4::version>(_archive, _version);

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
            });
        },
    };

    std::visit(visiter, _archive);
}

ArchiveVersion RsmArchive::get_version() const noexcept
{
    return _version;
}

const UnderlyingArchive &RsmArchive::get_archive() const noexcept
{
    return _archive;
}
} // namespace btu::bsa::detail
