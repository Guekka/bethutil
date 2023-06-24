#include "btu/bsa/archive.hpp"

#include <btu/common/filesystem.hpp>
#include <btu/common/functional.hpp>
#include <flow.hpp>

#include <fstream>

namespace btu::bsa {
File::File(ArchiveVersion v)
    : ver_(v)
{
    file_ = [v]() -> UnderlyingFile {
        switch (v)
        {
            case btu::bsa::ArchiveVersion::tes3: return libbsa::tes3::file{};
            case btu::bsa::ArchiveVersion::tes4: [[fallthrough]];
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
    , ver_(std::move(v))
{
}

auto File::compressed() const noexcept -> bool
{
    constexpr auto visitor = btu::common::overload{
        [](const libbsa::tes3::file &) { return false; },
        [](const libbsa::tes4::file &f) { return f.compressed(); },
        [](const libbsa::fo4::file &f) { return flow::any(f, &libbsa::fo4::chunk::compressed); },
    };

    return std::visit(visitor, file_);
}

void File::decompress()
{
    const auto visitor = btu::common::overload{
        [&](libbsa::tes3::file &) {},
        [&, this](libbsa::tes4::file &f) { f.decompress(static_cast<libbsa::tes4::version>(ver_)); },
        [](libbsa::fo4::file &f) { flow::for_each(f, [](auto &c) { c.decompress(); }); },
    };

    std::visit(visitor, file_);
    assert(!compressed());
}

void File::compress()
{
    const auto visitor = btu::common::overload{
        [&](libbsa::tes3::file &) {},
        [&, this](libbsa::tes4::file &f) { f.compress(static_cast<libbsa::tes4::version>(ver_)); },
        [](libbsa::fo4::file &f) { flow::for_each(f, [](auto &c) { c.compress(); }); },
    };

    std::visit(visitor, file_);
    assert(compressed());
}

void File::read(Path path)
{
    const auto visitor = btu::common::overload{
        [&](libbsa::tes3::file &f) { f.read(std::move(path)); },
        [&, this](libbsa::tes4::file &f) {
            f.read(std::move(path), static_cast<libbsa::tes4::version>(ver_));
        },
        [&, this](libbsa::fo4::file &f) { f.read(std::move(path), static_cast<libbsa::fo4::format>(ver_)); },
    };

    std::visit(visitor, file_);
}

void File::read(std::span<std::byte> src)
{
    const auto visitor = btu::common::overload{
        [&](libbsa::tes3::file &f) { f.read(src); },
        [&, this](libbsa::tes4::file &f) { f.read(src, static_cast<libbsa::tes4::version>(ver_)); },
        [&, this](libbsa::fo4::file &f) { f.read(src, static_cast<libbsa::fo4::format>(ver_)); },
    };

    std::visit(visitor, file_);
}

void File::write(Path path) const
{
    const auto visitor = btu::common::overload{
        [&](const libbsa::tes3::file &f) { f.write(std::move(path)); },
        [&, this](const libbsa::tes4::file &f) {
            f.write(std::move(path), static_cast<libbsa::tes4::version>(ver_));
        },
        [&, this](const libbsa::fo4::file &f) {
            f.write(std::move(path), static_cast<libbsa::fo4::format>(ver_));
        },
    };

    std::visit(visitor, file_);
}

void File::write(binary_io::any_ostream &os) const
{
    const auto visitor = btu::common::overload{
        [&](const libbsa::tes3::file &f) { f.write(os); },
        [&, this](const libbsa::tes4::file &f) { f.write(os, static_cast<libbsa::tes4::version>(ver_)); },
        [&, this](const libbsa::fo4::file &f) { f.write(os, static_cast<libbsa::fo4::format>(ver_)); },
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
                    res.emplace(std::move(str),
                                File(std::move(file.second), static_cast<ArchiveVersion>(ver)));
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

void write_archive(Archive arch, Path path)
{
    if (arch.empty())
        return;

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
            bsa.write(std::move(path));
            return;
        }
        case btu::bsa::ArchiveVersion::tes4: [[fallthrough]];
        case btu::bsa::ArchiveVersion::tes5: [[fallthrough]];
        case btu::bsa::ArchiveVersion::sse:
        {
            libbsa::tes4::archive bsa;
            auto flags = libbsa::tes4::archive_flag::directory_strings
                         | libbsa::tes4::archive_flag::file_strings;

            for (auto &&elem : std::move(arch))
            {
                auto elem_path = Path(elem.first);
                const auto d   = [&]() {
                    const auto key = elem_path.parent_path().lexically_normal().generic_string();
                    if (bsa.find(key) == bsa.end())
                        bsa.insert(key, libbsa::tes4::directory());
                    return bsa[key];
                }();

                if (elem.second.compressed())
                    flags |= libbsa::tes4::archive_flag::compressed;

                d->insert(elem_path.filename().lexically_normal().generic_string(),
                          std::move(elem.second).as_raw_file<libbsa::tes4::file>());
            }
            bsa.archive_flags(flags);
            bsa.write(std::move(path), static_cast<libbsa::tes4::version>(version));
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
            ba2.write(std::move(path), static_cast<libbsa::fo4::format>(version));
            return;
        }
    }
    libbsa::detail::declare_unreachable();
}

} // namespace btu::bsa
