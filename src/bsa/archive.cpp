#include "btu/bsa/archive.hpp"

#include <binary_io/memory_stream.hpp>
#include <btu/common/filesystem.hpp>
#include <btu/common/functional.hpp>
#include <flux.hpp>

#include <filesystem>
#include <fstream>

namespace btu::bsa {
File::File(ArchiveVersion v)
    : ver_(v)
{
    file_ = [v]() -> UnderlyingFile {
        switch (v)
        {
            case btu::bsa::ArchiveVersion::tes3: return libbsa::tes3::file{};
            case btu::bsa::ArchiveVersion::tes4:
            case btu::bsa::ArchiveVersion::tes5: [[fallthrough]];
            case btu::bsa::ArchiveVersion::sse: return libbsa::tes4::file{};
            case btu::bsa::ArchiveVersion::fo4: [[fallthrough]];
            case btu::bsa::ArchiveVersion::fo4dx: return libbsa::fo4::file{};
        }
        libbsa::detail::declare_unreachable();
    }();
}

File::File(UnderlyingFile f, ArchiveVersion v)
    : ver_(v)
    , file_(std::move(f))
{
}

auto File::compressed() const noexcept -> Compression
{
    constexpr auto visitor = btu::common::overload{
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
    constexpr auto visitor = btu::common::overload{
        [](const libbsa::tes3::file &f) { return f.size(); },
        [](const libbsa::tes4::file &f) { return f.size(); },
        [](const libbsa::fo4::file &f) { return flux::ref(f).map(&libbsa::fo4::chunk::size).sum(); },
    };

    return std::visit(visitor, file_);
}

void File::decompress()
{
    const auto visitor = btu::common::overload{
        [](libbsa::tes3::file &) {},
        [this](libbsa::tes4::file &f) { f.decompress(static_cast<libbsa::tes4::version>(ver_)); },
        [](libbsa::fo4::file &f) { flux::for_each(f, [](auto &c) { c.decompress(); }); },
    };

    std::visit(visitor, file_);
    assert(compressed() == Compression::No);
}

void File::compress()
{
    const auto visitor = btu::common::overload{
        [](libbsa::tes3::file &) {},
        [this](libbsa::tes4::file &f) { f.compress(static_cast<libbsa::tes4::version>(ver_)); },
        [](libbsa::fo4::file &f) { flux::for_each(f, [](auto &c) { c.compress(); }); },
    };

    std::visit(visitor, file_);
    assert(compressed() == Compression::Yes);
}

void File::read(Path path)
{
    const auto visitor = btu::common::overload{
        [&path](libbsa::tes3::file &f) { f.read(std::move(path)); },
        [&path, this](libbsa::tes4::file &f) {
            f.read(std::move(path), static_cast<libbsa::tes4::version>(ver_));
        },
        [&path, this](libbsa::fo4::file &f) {
            f.read(std::move(path), static_cast<libbsa::fo4::format>(ver_));
        },
    };

    std::visit(visitor, file_);
}

void File::read(std::span<std::byte> src)
{
    const auto visitor = btu::common::overload{
        [&](libbsa::tes3::file &f) { f.read(src); },
        [&src, this](libbsa::tes4::file &f) { f.read(src, static_cast<libbsa::tes4::version>(ver_)); },
        [&src, this](libbsa::fo4::file &f) { f.read(src, static_cast<libbsa::fo4::format>(ver_)); },
    };

    std::visit(visitor, file_);
}

void File::write(Path path) const
{
    const auto visitor = btu::common::overload{
        [&](const libbsa::tes3::file &f) { f.write(std::move(path)); },
        [&path, this](const libbsa::tes4::file &f) {
            f.write(std::move(path), static_cast<libbsa::tes4::version>(ver_));
        },
        [&path, this](const libbsa::fo4::file &f) {
            f.write(std::move(path), static_cast<libbsa::fo4::format>(ver_));
        },
    };

    std::visit(visitor, file_);
}

void File::write(binary_io::any_ostream &os) const
{
    const auto visitor = btu::common::overload{
        [&](const libbsa::tes3::file &f) { f.write(os); },
        [&os, this](const libbsa::tes4::file &f) { f.write(os, static_cast<libbsa::tes4::version>(ver_)); },
        [&os, this](const libbsa::fo4::file &f) { f.write(os, static_cast<libbsa::fo4::format>(ver_)); },
    };

    std::visit(visitor, file_);
}

auto File::version() const noexcept -> ArchiveVersion
{
    return ver_;
}

Archive::Archive(ArchiveVersion ver, ArchiveType type)
    : ver_(ver)
    , type_(type)
{
}

auto Archive::read(Path path) -> std::optional<Archive>
{
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

                res.emplace(common::as_ascii_string(std::move(relative_file_path)),
                            File(std::move(file), ArchiveVersion::tes3));
            }
            return res;
        }
        case libbsa::file_format::tes4:
        {
            libbsa::tes4::archive arch;
            res.ver_ = static_cast<ArchiveVersion>(arch.read(path));

            // Here we can't know easily if it's standard or textures. Because they're basically the same in SSE
            // Let's rely on the name of the archive. The only tes4 texture archive is x - Textures.bsa
            // TODO: that doesn't feel clean, it's a "magic string"
            res.type_ = ArchiveType::Standard;
            if (path.filename().u8string().ends_with(u8" - Textures.bsa"))
                res.type_ = ArchiveType::Textures;

            for (auto &&dir : std::move(arch))
            {
                for (auto &&file : std::move(dir.second))
                {
                    const auto u8str = virtual_to_local_path(dir.first, file.first);
                    const auto str   = btu::common::as_ascii_string(u8str);
                    res.emplace(str, File(std::move(file.second), res.ver_));
                }
            }
            return res;
        }
        case libbsa::file_format::fo4:
        {
            libbsa::fo4::archive arch;
            res.ver_ = static_cast<ArchiveVersion>(arch.read(std::move(path)));

            res.type_ = res.ver_ == ArchiveVersion::fo4 ? ArchiveType::Standard : ArchiveType::Textures;

            for (auto &&[key, file] : std::move(arch))
            {
                auto relative_file_path = virtual_to_local_path(key);
                res.emplace(common::as_ascii_string(std::move(relative_file_path)),
                            File(std::move(file), res.ver_));
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
void do_write(Archive &&arch, WriteFunc &&write_func, fs::path path)
    requires std::is_rvalue_reference_v<decltype(arch)> && std::is_invocable_v<WriteFunc, Archive, fs::path>
{
    auto write_and_check = [&](fs::path p) {
        std::forward<WriteFunc>(write_func)(BTU_MOV(arch), p);
        arch.clear(); // release memory mapping
        if (!fs::exists(p))
        {
            throw std::runtime_error("Failed to write archive to " + p.string());
        }
    };

    // On Windows, we cannot remove the file while it is memory mapped
    // So we need to release the memory mapping first
    // That's why we have to move the archive to the lambda
    if (fs::exists(path))
    {
        auto tmp_path = path.parent_path() / (path.filename().u8string() + u8".tmp");
        write_and_check(tmp_path);
        fs::remove(path);
        fs::rename(tmp_path, path);
    }
    else
    {
        write_and_check(path);
    }
}

void Archive::write(btu::Path path) &&
{
    if (files_.empty())
        return;

    fs::create_directories(path.parent_path());

    switch (ver_)
    {
        case btu::bsa::ArchiveVersion::tes3:
        {
            libbsa::tes3::archive bsa;
            for (auto &&elem : std::move(files_))
            {
                bsa.insert(elem.first, std::move(elem.second).as_raw_file<libbsa::tes3::file>());
            }
            do_write(BTU_MOV(bsa), [](auto &&bsa, auto &&path) { bsa.write(BTU_FWD(path)); }, BTU_MOV(path));
            return;
        }
        case btu::bsa::ArchiveVersion::tes4:
        case btu::bsa::ArchiveVersion::tes5: [[fallthrough]];
        case btu::bsa::ArchiveVersion::sse:
        {
            libbsa::tes4::archive bsa;

            for (auto &&elem : std::move(files_))
            {
                auto elem_path = Path(elem.first);
                const auto d   = [&]() {
                    const auto key = elem_path.parent_path().lexically_normal().generic_string();
                    if (bsa.find(key) == bsa.end())
                        bsa.insert(key, libbsa::tes4::directory());
                    return bsa[key];
                }();

                d->insert(elem_path.filename().lexically_normal().generic_string(),
                          std::move(elem.second).as_raw_file<libbsa::tes4::file>());
            }

            bsa.archive_flags(libbsa::tes4::archive_flag::directory_strings
                              | libbsa::tes4::archive_flag::file_strings);

            do_write(
                BTU_MOV(bsa),
                [this](auto &&bsa, auto &&path) {
                    bsa.write(BTU_FWD(path), static_cast<libbsa::tes4::version>(ver_));
                },
                BTU_MOV(path));
            return;
        }
        case btu::bsa::ArchiveVersion::fo4: [[fallthrough]];
        case btu::bsa::ArchiveVersion::fo4dx:
        {
            libbsa::fo4::archive ba2;
            for (auto &&elem : std::move(files_))
            {
                ba2.insert(elem.first, std::move(elem.second).as_raw_file<libbsa::fo4::file>());
            }
            do_write(
                BTU_MOV(ba2),
                [this](auto &&ba2, auto &&path) {
                    ba2.write(BTU_FWD(path), static_cast<libbsa::fo4::format>(ver_));
                },
                BTU_MOV(path));

            return;
        }
    }
    libbsa::detail::declare_unreachable();
}

void Archive::set_version(ArchiveVersion version) noexcept
{
    if (version == std::exchange(ver_, version))
        return;

    btu::common::for_each_mt(files_, [version](auto &path_file) {
        auto res_file = bsa::File(version);

        auto buffer = binary_io::any_ostream{binary_io::memory_ostream{}};

        path_file.second.write(buffer);
        res_file.read(buffer.get<binary_io::memory_ostream>().rdbuf());

        if (path_file.second.compressed() == Compression::Yes)
            res_file.compress();

        path_file.second = BTU_MOV(res_file);
    });
}

auto Archive::file_size() const noexcept -> size_t
{
    return flux::from_range(files_).map([](const auto &pair) { return pair.second.size(); }).sum();
}

void Archive::emplace(std::string name, File file)
{
    if (file.version() != ver_)
        throw std::invalid_argument("File version does not match archive version");

    files_.insert_or_assign(std::move(name), std::move(file));
}

auto Archive::get(const std::string &name) -> File &
{
    return files_.try_emplace(name, ver_).first->second;
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
