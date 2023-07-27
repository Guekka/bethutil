#include "btu/bsa/archive.hpp"

#include <btu/common/filesystem.hpp>
#include <btu/common/functional.hpp>
#include <flow.hpp>

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
    : file_(std::move(f))
    , ver_(v)
{
}

auto File::compressed() const noexcept -> Compression
{
    constexpr auto visitor = btu::common::overload{
        [](const libbsa::tes3::file &) { return Compression::No; },
        [](const libbsa::tes4::file &f) { return f.compressed() ? Compression::Yes : Compression::No; },
        [](const libbsa::fo4::file &f) {
            return flow::any(f, &libbsa::fo4::chunk::compressed) ? Compression::Yes : Compression::No;
        },
    };

    return std::visit(visitor, file_);
}

auto File::size() const noexcept -> size_t
{
    constexpr auto visitor = btu::common::overload{
        [](const libbsa::tes3::file &f) { return f.size(); },
        [](const libbsa::tes4::file &f) { return f.size(); },
        [](const libbsa::fo4::file &f) { return flow::from(f).map(&libbsa::fo4::chunk::size).sum(); },
    };

    return std::visit(visitor, file_);
}

void File::decompress()
{
    const auto visitor = btu::common::overload{
        [](libbsa::tes3::file &) {},
        [this](libbsa::tes4::file &f) { f.decompress(static_cast<libbsa::tes4::version>(ver_)); },
        [](libbsa::fo4::file &f) { flow::for_each(f, [](auto &c) { c.decompress(); }); },
    };

    std::visit(visitor, file_);
    assert(compressed() == Compression::No);
}

void File::compress()
{
    const auto visitor = btu::common::overload{
        [](libbsa::tes3::file &) {},
        [this](libbsa::tes4::file &f) { f.compress(static_cast<libbsa::tes4::version>(ver_)); },
        [](libbsa::fo4::file &f) { flow::for_each(f, [](auto &c) { c.compress(); }); },
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

auto read_archive(Path path) -> std::optional<Archive>
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
            auto ver = arch.read(std::move(path));
            for (auto &&dir : std::move(arch))
            {
                for (auto &&file : std::move(dir.second))
                {
                    const auto u8str = virtual_to_local_path(dir.first, file.first);
                    const auto str   = btu::common::as_ascii_string(u8str);
                    res.emplace(str, File(std::move(file.second), static_cast<ArchiveVersion>(ver)));
                }
            }
            return res;
        }
        case libbsa::file_format::fo4:
        {
            libbsa::fo4::archive arch;
            auto ver = arch.read(std::move(path));
            for (auto &&[key, file] : std::move(arch))
            {
                auto relative_file_path = virtual_to_local_path(key);
                res.emplace(common::as_ascii_string(std::move(relative_file_path)),
                            File(std::move(file), static_cast<ArchiveVersion>(ver)));
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
void do_write(Archive &&arch, WriteFunc &&write_func, fs::path path) requires
    std::is_rvalue_reference_v<decltype(arch)> && std::is_invocable_v<WriteFunc, Archive, fs::path>
{
    auto write_and_check = [&](fs::path p) {
        std::forward<WriteFunc>(write_func)(BTU_MOV(arch), p);
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

void write_archive(Archive &&arch, Path path)
{
    if (arch.empty())
        return;

    fs::create_directories(path.parent_path());

    auto version = arch.begin()->second.version();
    switch (version)
    {
        case btu::bsa::ArchiveVersion::tes3:
        {
            libbsa::tes3::archive bsa;
            for (auto &&elem : std::move(arch))
            {
                bsa.insert(elem.first, std::move(elem.second).as_raw_file<libbsa::tes3::file>());
            }
            do_write(
                BTU_MOV(bsa), [](auto &&bsa, auto &&path) { bsa.write(BTU_FWD(path)); }, BTU_MOV(path));
            return;
        }
        case btu::bsa::ArchiveVersion::tes4:
        case btu::bsa::ArchiveVersion::tes5: [[fallthrough]];
        case btu::bsa::ArchiveVersion::sse:
        {
            libbsa::tes4::archive bsa;

            for (auto &&elem : std::move(arch))
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
                [version](auto &&bsa, auto &&path) {
                    bsa.write(BTU_FWD(path), static_cast<libbsa::tes4::version>(version));
                },
                BTU_MOV(path));
            return;
        }
        case btu::bsa::ArchiveVersion::fo4: [[fallthrough]];
        case btu::bsa::ArchiveVersion::fo4dx:
        {
            libbsa::fo4::archive ba2;
            for (auto &&elem : std::move(arch))
            {
                ba2.insert(elem.first, std::move(elem.second).as_raw_file<libbsa::fo4::file>());
            }
            do_write(
                BTU_MOV(ba2),
                [version](auto &&ba2, auto &&path) {
                    ba2.write(BTU_FWD(path), static_cast<libbsa::fo4::format>(version));
                },
                BTU_MOV(path));

            return;
        }
    }
    libbsa::detail::declare_unreachable();
}

void test()
{
    std::string str = "Hello, world!\n";
    std::vector<std::string> messages;
    messages.emplace_back(BTU_MOV(str));
    std::cout << str;
}

} // namespace btu::bsa
