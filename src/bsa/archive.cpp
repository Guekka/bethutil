#include "btu/bsa/archive.hpp"

#include <btu/common/filesystem.hpp>
#include <btu/common/functional.hpp>

#include <execution>
#include <fstream>

namespace btu::bsa {

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

Archive::Archive(const Path &a_path)
{
    read(a_path);
}

Archive::Archive(ArchiveVersion a_version, bool a_compressed)
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

auto Archive::read(Path a_path) -> ArchiveVersion
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

auto Archive::write(Path a_path) -> void
{
    const auto writer = btu::common::overload{
        [&](libbsa::tes3::archive &bsa) { bsa.write(a_path); },
        [&](libbsa::tes4::archive &bsa) {
            const auto version = archive_version<libbsa::tes4::version>(archive_, version_);
            bsa.write(a_path, version);
        },
        [&](libbsa::fo4::archive &ba2) {
            const auto version = archive_version<libbsa::fo4::format>(archive_, version_);
            ba2.write(a_path, version);
        },
    };

    std::visit(writer, archive_);
}

void Archive::add_file(const common::Path &a_relative, UnderlyingFile file)
{
    std::scoped_lock lock(mutex_);
    const auto adder = btu::common::overload{
        [&](libbsa::tes3::archive &bsa, libbsa::tes3::file &&f) {
            bsa.insert(a_relative.generic_string(), std::move(f));
        },
        [&](libbsa::tes4::archive &bsa, libbsa::tes4::file &&f) {
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
        [&](libbsa::fo4::archive &ba2, libbsa::fo4::file &&f) {
            ba2.insert(a_relative.lexically_normal().generic_string(), std::move(f));
        },
        [](auto &&, auto &&) { throw libbsa::exception("Trying to add x type file to y type bsa"); }};

    std::visit(adder, archive_, std::move(file));
}

auto Archive::add_file(const Path &a_root, const Path &a_path) -> void
{
    const auto relative = a_path.lexically_relative(a_root).lexically_normal();
    const auto compress = compressed_ ? libbsa::compression_type::compressed
                                      : libbsa::compression_type::decompressed;

    const auto adder = btu::common::overload{
        [&](libbsa::tes3::archive &) {
            libbsa::tes3::file f;
            f.read(a_path);
            add_file(relative, std::move(f));
        },
        [&, this](libbsa::tes4::archive &) {
            libbsa::tes4::file f;
            const auto version = archive_version<libbsa::tes4::version>(archive_, version_);
            f.read(a_path, version, libbsa::tes4::compression_codec::normal, compress);
            add_file(relative, std::move(f));
        },
        [&, this](libbsa::fo4::archive &) {
            libbsa::fo4::file f;
            const auto format = archive_version<libbsa::fo4::format>(archive_, version_);
            f.read(a_path,
                   format,
                   512u,
                   512u, // Default
                   libbsa::fo4::compression_level::normal,
                   compress);
            add_file(relative, std::move(f));
        },
    };
    std::visit(adder, archive_);
}

auto Archive::add_file(const Path &relative, std::vector<std::byte> a_data) -> void
{
    const auto adder = btu::common::overload{
        [&](libbsa::tes3::archive &) {
            libbsa::tes3::file f;
            f.set_data(std::move(a_data));
            add_file(relative, std::move(f));
        },
        [&, this](libbsa::tes4::archive &) {
            libbsa::tes4::file f;
            const auto version = archive_version<libbsa::tes4::version>(archive_, version_);
            f.set_data(std::move(a_data));
            if (compressed_)
                f.compress(version);
            add_file(relative, std::move(f));
        },
        [&, this](libbsa::fo4::archive &) {
            libbsa::fo4::file f;
            const auto compress = compressed_ ? libbsa::compression_type::compressed
                                              : libbsa::compression_type::decompressed;
            const auto format   = archive_version<libbsa::fo4::format>(archive_, version_);
            f.read(a_data,
                   format,
                   512u,
                   512u, // Default
                   libbsa::fo4::compression_level::normal,
                   compress);
            add_file(relative, std::move(f));
        },
    };

    std::visit(adder, archive_);
}

auto Archive::write_file(const UnderlyingFile &file, const Path &path) -> void
{
    auto make_dir = [](const Path &p) {
        const auto dir = p.parent_path();
        if (!std::filesystem::exists(dir))
            std::filesystem::create_directories(dir);
    };

    auto visiter = btu::common::overload{
        [&](const libbsa::tes3::file &file) {
            make_dir(path);
            file.write(path);
        },
        [&](const libbsa::tes4::file &file) {
            const auto ver = archive_version<libbsa::tes4::version>(archive_, version_);
            make_dir(path);
            file.write(path, ver);
        },
        [&](const libbsa::fo4::file &file) {
            const auto ver = archive_version<libbsa::fo4::format>(archive_, version_);
            make_dir(path);
            file.write(path, ver);
        },
    };

    std::visit(visiter, file);
}

auto Archive::unpack(const Path &out_path) -> void
{
    auto visiter = btu::common::overload{
        [&](libbsa::tes3::archive &bsa) {
            btu::common::for_each_mt(bsa, [&](auto &&pair) {
                const auto [key, file] = pair;
                write_file(file, out_path / virtual_to_local_path(key));
            });
        },
        [&](libbsa::tes4::archive &bsa) {
            for (auto &dir : bsa)
            {
                btu::common::for_each_mt(dir.second, [&](auto &&pair) {
                    const auto [key, file] = pair;
                    write_file(file, out_path / virtual_to_local_path(dir.first, key));
                });
            }
        },
        [&](libbsa::fo4::archive &ba2) {
            btu::common::for_each_mt(ba2, [&](auto &&pair) {
                auto &&[key, file] = pair;
                write_file(file, out_path / virtual_to_local_path(key));
            });
        },
    };

    std::visit(visiter, archive_);
}

void Archive::unpack_file(const common::Path &rel_path, const common::Path &out_path)
{
    auto visiter = btu::common::overload{
        [&](libbsa::tes3::archive &bsa) {
            auto f = UnderlyingFile{*bsa[rel_path.string()]};
            write_file(f, out_path);
        },
        [&](libbsa::tes4::archive &bsa) {
            auto dir   = rel_path.parent_path();
            auto fname = rel_path.filename();
            auto f     = UnderlyingFile{*bsa[dir.string()][fname.string()]};
            write_file(f, out_path);
        },
        [&](libbsa::fo4::archive &ba2) {
            auto f = UnderlyingFile{*ba2[rel_path.string()]};
            write_file(f, out_path);
        },
    };

    std::visit(visiter, archive_);
}

auto Archive::file_count() const noexcept -> size_t
{
    return std::visit([](auto &&arch) { return arch.size(); }, archive_);
}

auto Archive::begin() -> Iterator
{
    // Stack corruption without constexpr.
    // See https://developercommunity.visualstudio.com/t/runtime-stack-corruption-using-stdvisit/346200
    constexpr auto visitor = btu::common::overload{
        [](libbsa::tes4::archive &a) { return Iterator(tes4Iter(a)); },
        [](auto &a) { return Iterator{a.begin()}; },
    };

    return std::visit(visitor, archive_);
}

auto Archive::end() -> Iterator
{
    // Stack corruption without constexpr.
    // See https://developercommunity.visualstudio.com/t/runtime-stack-corruption-using-stdvisit/346200
    constexpr auto visiter = btu::common::overload{
        [&](libbsa::tes4::archive &a) { return Iterator(tes4Iter::end(a)); },
        [&](auto &a) { return Iterator(a.end()); },
    };

    return std::visit(visiter, archive_);
}

auto Archive::get_version() const noexcept -> ArchiveVersion
{
    return version_;
}

auto Archive::get_archive() const noexcept -> const UnderlyingArchive &
{
    return archive_;
}

tes4Iter::tes4Iter(libbsa::tes4::archive &arch) noexcept
    : dir_(arch.begin())
    , dir_end_(arch.end())
    , file_(arch.empty() ? libbsa::tes4::archive::mapped_type::iterator{} : arch.begin()->second.begin())
{
}

auto tes4Iter::end(libbsa::tes4::archive &arch) -> tes4Iter
{
    auto it = tes4Iter(arch);
    it.dir_ = it.dir_end_;
    return it;
}

auto tes4Iter::operator*() noexcept -> std::string
{
    return btu::common::as_ascii_string(virtual_to_local_path(dir_->first, file_->first));
}

auto tes4Iter::operator++() noexcept -> tes4Iter &
{
    ++file_;
    if (file_ == dir_->second.end())
    {
        ++dir_;
        if (dir_ != dir_end_)
            file_ = dir_->second.begin();
    }
    return *this;
}

auto tes4Iter::operator==(tes4Iter other) const noexcept -> bool
{
    // If we're at end, only compare dir
    return dir_ == other.dir_ && (dir_ == dir_end_ || file_ == other.file_);
}

Archive::Iterator::Iterator(UnderlyingIterator it) noexcept
    : it_(it)
{
}

auto Archive::Iterator::operator*() noexcept -> std::string
{
    return std::visit(btu::common::overload{[](tes4Iter &i) { return *i; },
                                            [](auto &i) { return std::string(i->first.name()); }},
                      it_);
}

auto Archive::Iterator::operator++() noexcept -> Archive::Iterator &
{
    std::visit([](auto &i) { ++i; }, it_);
    return *this;
}

auto Archive::Iterator::operator==(Iterator other) const noexcept -> bool
{
    return it_ == other.it_;
}

} // namespace btu::bsa
