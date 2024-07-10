#include "btu/bsa/archive.hpp"

#include "bsa/detail/common.hpp"

#include <binary_io/memory_stream.hpp>
#include <bsa/bsa.hpp>
#include <btu/common/threading.hpp>
#include <flux.hpp>

#include <filesystem>
#include <utility>

namespace btu::bsa {

[[nodiscard]] constexpr auto to_tes4_version(ArchiveVersion version) noexcept
    -> std::optional<libbsa::tes4::version>
{
    switch (version)
    {
        case ArchiveVersion::tes4: return libbsa::tes4::version::tes4;
        case ArchiveVersion::fo3: return libbsa::tes4::version::fo3;
        case ArchiveVersion::sse: return libbsa::tes4::version::sse;
        case ArchiveVersion::tes5: return libbsa::tes4::version::tes5;
        case ArchiveVersion::tes3:
        case ArchiveVersion::fo4:
        case ArchiveVersion::starfield: return std::nullopt;
    }
    libbsa::detail::declare_unreachable();
}

[[nodiscard]] constexpr auto from_tes4_version(libbsa::tes4::version version) noexcept -> ArchiveVersion
{
    switch (version)
    {
        case libbsa::tes4::version::tes4: return ArchiveVersion::tes4;
        case libbsa::tes4::version::fo3: return ArchiveVersion::fo3;
        case libbsa::tes4::version::sse: return ArchiveVersion::sse;
    }
    libbsa::detail::declare_unreachable();
}

[[nodiscard]] constexpr auto to_fo4_format(ArchiveVersion version,
                                           ArchiveType type) noexcept -> std::optional<libbsa::fo4::format>
{
    switch (version)
    {
        case ArchiveVersion::fo4: [[fallthrough]];
        case ArchiveVersion::starfield:
        {
            switch (type)
            {
                case ArchiveType::Textures: return libbsa::fo4::format::directx;
                case ArchiveType::Standard: return libbsa::fo4::format::general;
            }
            [[fallthrough]];
        }
        default: return std::nullopt;
    }
}

[[nodiscard]] constexpr auto fo4_compression_format(ArchiveVersion version, ArchiveType type)
{
    if (version == ArchiveVersion::starfield && type == ArchiveType::Textures)
        return libbsa::fo4::compression_format::lz4;

    return libbsa::fo4::compression_format::zip;
}

File::File(ArchiveVersion version, ArchiveType type, std::optional<TES4ArchiveType> tes4_type = std::nullopt)
    : ver_(version)
    , type_(type)
    , tes4_archive_type_(tes4_type)
{
    file_ = [version]() -> UnderlyingFile {
        switch (version)
        {
            case ArchiveVersion::tes3: return libbsa::tes3::file{};
            case ArchiveVersion::tes4:
            case ArchiveVersion::fo3:
            case ArchiveVersion::tes5: [[fallthrough]];
            case ArchiveVersion::sse: return libbsa::tes4::file{};
            case ArchiveVersion::fo4: [[fallthrough]];
            case ArchiveVersion::starfield: return libbsa::fo4::file{};
        }
        libbsa::detail::declare_unreachable();
    }();
}

File::File(UnderlyingFile f, ArchiveVersion version, ArchiveType type, std::optional<TES4ArchiveType> tes4_type = std::nullopt)
    : ver_(version)
    , type_(type)
    , tes4_archive_type_(tes4_type)
    , file_(std::move(f))
{
}

auto File::compressed() const noexcept -> Compression
{
    constexpr auto visitor = common::Overload{
        [](const libbsa::tes3::file &) { return Compression::No; },
        [](const libbsa::tes4::file &f) { return f.compressed() ? Compression::Yes : Compression::No; },
        [](const libbsa::fo4::file &f) {
            return flux::any(f, &libbsa::fo4::chunk::compressed) ? Compression::Yes : Compression::No;
        },
    };

    return std::visit(visitor, file_);
}

auto File::size() const noexcept -> size_t
{
    constexpr auto visitor = common::Overload{
        [](const libbsa::tes3::file &f) { return f.size(); },
        [](const libbsa::tes4::file &f) { return f.size(); },
        [](const libbsa::fo4::file &f) { return flux::ref(f).map(&libbsa::fo4::chunk::size).sum(); },
    };

    return std::visit(visitor, file_);
}

void File::compress()
{
    const auto visitor = common::Overload{
        [](libbsa::tes3::file &) {},
        [this](libbsa::tes4::file &f) { f.compress({.version_ = *to_tes4_version(ver_)}); },
        [this](libbsa::fo4::file &f) {
            flux::for_each(f, [this](auto &c) {
                const auto comp_level = [this] {
                    switch (ver_)
                    {
                        case ArchiveVersion::fo4: return libbsa::fo4::compression_level::fo4;
                        case ArchiveVersion::starfield: return libbsa::fo4::compression_level::sf;
                        default: return libbsa::fo4::compression_level::fo4;
                    };
                }();

                c.compress({
                    .compression_format_ = fo4_compression_format(ver_, type_),
                    .compression_level_  = comp_level,
                });
            });
        },
    };

    std::visit(visitor, file_);
    assert(compressed() == Compression::Yes);
}

auto File::read(Path path) -> bool
{
    const auto visitor = common::Overload{
        [&path](libbsa::tes3::file &f) { f.read(std::move(path)); },
        [&path, this](libbsa::tes4::file &f) {
            f.read(libbsa::read_source(std::move(path)), {.version_ = *to_tes4_version(ver_)});
        },
        [&path, this](libbsa::fo4::file &f) {
            f.read(std::move(path), {.format_ = *to_fo4_format(ver_, type_)});
        },
    };

    try
    {
        std::visit(visitor, file_);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

auto File::read(std::span<std::byte> src) -> bool
{
    const auto visitor = common::Overload{
        [&](libbsa::tes3::file &f) { f.read(libbsa::read_source(src)); },
        [&src, this](libbsa::tes4::file &f) {
            f.read(libbsa::read_source(src), {.version_ = *to_tes4_version(ver_)});
        },
        [&src, this](libbsa::fo4::file &f) {
            f.read(libbsa::read_source(src), {.format_ = *to_fo4_format(ver_, type_)});
        },
    };

    try
    {
        std::visit(visitor, file_);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

auto File::write(Path path) const -> bool
{
    const auto visitor = common::Overload{
        [&](const libbsa::tes3::file &f) { f.write(std::move(path)); },
        [&path, this](const libbsa::tes4::file &f) {
            f.write(std::move(path), {.version_ = *to_tes4_version(ver_)});
        },
        [&path, this](const libbsa::fo4::file &f) {
            f.write(std::move(path), {.format_ = *to_fo4_format(ver_, type_)});
        },
    };

    try
    {
        std::visit(visitor, file_);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

auto File::write(binary_io::any_ostream &dst) const -> bool
{
    const auto visitor = common::Overload{
        [&](const libbsa::tes3::file &f) { f.write(dst); },
        [&dst, this](const libbsa::tes4::file &f) { f.write(dst, {.version_ = *to_tes4_version(ver_)}); },
        [&dst, this](const libbsa::fo4::file &f) { f.write(dst, {.format_ = *to_fo4_format(ver_, type_)}); },
    };

    try
    {
        std::visit(visitor, file_);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

auto File::version() const noexcept -> ArchiveVersion
{
    return ver_;
}

auto File::type() const noexcept -> ArchiveType
{
    return type_;
}

auto File::tes4_archive_type() const noexcept -> std::optional<TES4ArchiveType>
{
    return tes4_archive_type_;
}

Archive::Archive(const ArchiveVersion ver, const ArchiveType type)
    : ver_(ver)
    , type_(type)
{
}

auto Archive::read(Path path) -> std::optional<Archive>
{
    if (!exists(path))
        return {};

    const auto opt_format = libbsa::guess_file_format(path);
    if (!opt_format.has_value())
        return {};

    const auto format = *opt_format;
    Archive res;
    switch (format)
    {
        case libbsa::file_format::tes3:
        {
            libbsa::tes3::archive arch;
            arch.read(std::move(path));

            res.ver_  = ArchiveVersion::tes3;
            res.type_ = ArchiveType::Standard;

            for (auto &&[key, file] : std::move(arch))
            {
                auto relative_file_path = virtual_to_local_path(key);

                const bool success = res.emplace(common::as_ascii_string(std::move(relative_file_path)),
                                                 File(std::move(file),
                                                      ArchiveVersion::tes3,
                                                      ArchiveType::Standard));

                assert(success && "Invalid archive file type, this should never happen");
            }
            return res;
        }
        case libbsa::file_format::tes4:
        {
            libbsa::tes4::archive arch;
            res.ver_ = from_tes4_version(arch.read(path));

            // Here we can't know easily if it's standard or textures. Because they're basically the same in SSE
            // Let's rely on the name of the archive. The only tes4 texture archive is x - Textures.bsa
            // TODO: that doesn't feel clean, it's a "magic string"
            res.type_ = ArchiveType::Standard;
            if (path.filename().u8string().ends_with(u8" - Textures.bsa"))
                res.type_ = ArchiveType::Textures;

            for (auto &[dir_path, dir] : std::move(arch))
            {
                for (auto &[file_path, file] : std::move(dir))
                {
                    const auto u8str = virtual_to_local_path(dir_path, file_path);
                    const auto str   = common::as_ascii_string(u8str);

                    const bool success = res.emplace(str, File(std::move(file), res.ver_, res.type_));
                    assert(success && "Invalid archive file type, this should never happen");
                }
            }
            return res;
        }
        case libbsa::file_format::fo4:
        {
            libbsa::fo4::archive arch;
            const auto archive_info = arch.read(std::move(path));

            res.ver_ = [&] {
                switch (archive_info.version_)
                {
                    case libbsa::fo4::version::v1: [[fallthrough]];
                    case libbsa::fo4::version::v7: [[fallthrough]];
                    case libbsa::fo4::version::v8: return ArchiveVersion::fo4;
                    case libbsa::fo4::version::v2: [[fallthrough]];
                    case libbsa::fo4::version::v3: return ArchiveVersion::starfield;
                }
                libbsa::detail::declare_unreachable();
            }();

            res.type_ = archive_info.format_ == ::bsa::fo4::format::directx ? ArchiveType::Textures
                                                                            : ArchiveType::Standard;

            for (auto &&[key, file] : std::move(arch))
            {
                auto relative_file_path = virtual_to_local_path(key);
                const bool success      = res.emplace(common::as_ascii_string(std::move(relative_file_path)),
                                                 File(std::move(file), res.ver_, res.type_));
                assert(success && "Invalid archive file type, this should never happen");
            }
            return res;
        }
    }
    libbsa::detail::declare_unreachable();
}

/**
 * Write data to a file at a specified path using a provided write function.
 *
 * @tparam Archive The data type to be written.
 * @tparam WriteFunc The type of the function to use for writing.
 *
 * @param arch The data to be written (rvalue reference).
 * @param write_func The function or callable object to use for writing.
 * @param path The path of the file to be written.
 */
template<typename Archive, typename WriteFunc>
[[nodiscard]] auto do_write(Archive &&arch, WriteFunc &&write_func, const fs::path &path) -> bool
    requires std::is_rvalue_reference_v<decltype(arch)> && std::is_invocable_v<WriteFunc, Archive, fs::path>
{
    auto write_and_check = [&](fs::path p) {
        std::forward<WriteFunc>(write_func)(BTU_MOV(arch), p);
        arch.clear(); // release memory mapping
        return exists(p);
    };

    // On Windows, we cannot remove the file while it is memory mapped
    // So we need to release the memory mapping first
    // That's why we have to move the archive to the lambda
    if (exists(path))
    {
        const auto tmp_path = path.parent_path() / (path.filename().u8string() + u8".tmp");
        write_and_check(tmp_path);
        fs::remove(path);
        fs::rename(tmp_path, path);
        return true;
    }

    return write_and_check(path);
}

auto Archive::write(Path path) && -> bool
{
    if (files_.empty())
        return false;

    create_directories(path.parent_path());

    switch (ver_)
    {
        case ArchiveVersion::tes3:
        {
            libbsa::tes3::archive bsa;
            for (auto &&elem : std::move(files_))
            {
                bsa.insert(elem.first, std::move(elem.second).as_raw_file<libbsa::tes3::file>());
            }
            return do_write(
                BTU_MOV(bsa), [](auto &&bsa, auto &&path) { bsa.write(BTU_FWD(path)); }, BTU_MOV(path));
        }
        case ArchiveVersion::tes4:
        case ArchiveVersion::fo3:
        case ArchiveVersion::tes5: [[fallthrough]];
        case ArchiveVersion::sse:
        {
            libbsa::tes4::archive bsa;

            for (auto &&elem : std::move(files_))
            {
                auto elem_path = Path(elem.first);
                const auto d   = [&] {
                    const auto key = elem_path.parent_path().lexically_normal().generic_string();
                    if (bsa.find(key) == bsa.end())
                        bsa.insert(key, libbsa::tes4::directory());
                    return bsa[key];
                }();

                auto result = d->insert(elem_path.filename().lexically_normal().generic_string(),
                          std::move(elem.second).as_raw_file<libbsa::tes4::file>());

                if (result.second)
                {
                    if (auto arch_type = elem.second.tes4_archive_type();
                        arch_type)
                    {
                        bsa.archive_types(bsa.archive_types() | arch_type.value());
                    }
                }
            }

            bsa.archive_flags(libbsa::tes4::archive_flag::directory_strings
                              | libbsa::tes4::archive_flag::file_strings);

            if (bsa.meshes())
                bsa.archive_flags(bsa.archive_flags()
                                  | libbsa::tes4::archive_flag::retain_strings_during_startup);
            if (bsa.textures())
                bsa.archive_flags(bsa.archive_flags() | libbsa::tes4::archive_flag::embedded_file_names);
            if (bsa.sounds())
                bsa.archive_flags(bsa.archive_flags() | libbsa::tes4::archive_flag::retain_file_names);

            return do_write(
                BTU_MOV(bsa),
                [this](auto &&bsa, auto &&path) { bsa.write(BTU_FWD(path), *to_tes4_version(ver_)); },
                BTU_MOV(path));
        }
        case ArchiveVersion::fo4: [[fallthrough]];
        case ArchiveVersion::starfield:
        {
            libbsa::fo4::archive ba2;
            for (auto &&elem : std::move(files_))
            {
                ba2.insert(elem.first, std::move(elem.second).as_raw_file<libbsa::fo4::file>());
            }
            return do_write(
                BTU_MOV(ba2),
                [this](auto &&ba2, auto &&path) {
                    const auto v = ver_ == ArchiveVersion::starfield ? libbsa::fo4::version::v3
                                                                     : libbsa::fo4::version::v8;

                    ba2.write(BTU_FWD(path),
                              {
                                  .format_             = *to_fo4_format(ver_, type_),
                                  .version_            = v,
                                  .compression_format_ = fo4_compression_format(ver_, type_),
                              });
                },
                BTU_MOV(path));
        }
    }
    libbsa::detail::declare_unreachable();
}

void Archive::set_version(ArchiveVersion version) noexcept
{
    if (version == std::exchange(ver_, version))
        return;

    common::for_each_mt(files_, [version](auto &path_file) {
        auto res_file = File(version, path_file.second.type());

        auto buffer = binary_io::any_ostream{binary_io::memory_ostream{}};

        bool res = path_file.second.write(buffer);
        // TODO: better error handling
        assert(res);
        res = res_file.read(buffer.get<binary_io::memory_ostream>().rdbuf());
        assert(res);

        if (path_file.second.compressed() == Compression::Yes)
            res_file.compress();

        path_file.second = BTU_MOV(res_file);
    });
}

auto Archive::file_size() const noexcept -> size_t
{
    return flux::from_range(files_).map([](const auto &pair) { return pair.second.size(); }).sum();
}

auto Archive::emplace(std::string name, File file) -> bool
{
    if (file.version() != ver_)
        return false;

    files_.insert_or_assign(std::move(name), std::move(file));
    return true;
}

auto Archive::get(const std::string &name) -> File &
{
    return files_.try_emplace(name, ver_, type_).first->second;
}
auto Archive::empty() const noexcept -> bool
{
    return files_.empty();
}

auto Archive::size() const noexcept -> size_t
{
    return files_.size();
}

} // namespace btu::bsa
