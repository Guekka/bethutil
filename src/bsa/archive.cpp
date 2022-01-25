#include "btu/bsa/archive.hpp"

#include <btu/common/filesystem.hpp>
#include <btu/common/functional.hpp>
#include <flow.hpp>

#include <execution>
#include <fstream>

namespace btu::bsa {
File::File(ArchiveVersion v)
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

File::File(UnderlyingFile f)
{
    file_ = std::move(f);
}

auto File::compressed() const noexcept -> bool
{
    const auto visitor = btu::common::overload{
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
}

void File::compress()
{
    const auto visitor = btu::common::overload{
        [&](libbsa::tes3::file &) {},
        [&, this](libbsa::tes4::file &f) { f.compress(static_cast<libbsa::tes4::version>(ver_)); },
        [](libbsa::fo4::file &f) { flow::for_each(f, [](auto &c) { c.compress(); }); },
    };

    std::visit(visitor, file_);
}

void File::read(std::filesystem::path path)
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

void File::write(std::filesystem::path path) const
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
            btu::common::for_each_mt(std::move(arch), [&](auto &&elem) {
                res.emplace(elem.first.name(), std::move(elem.second));
            });
            return res;
        }
        case libbsa::file_format::tes4:
        {
            libbsa::tes4::archive arch;
            arch.read(std::move(path));
            btu::common::for_each_mt(std::move(arch), [&](auto &&dir) {
                for (auto &&file : std::move(dir.second))
                {
                    const auto u8str = virtual_to_local_path(dir.first, file.first);
                    const auto str   = btu::common::as_ascii_string(u8str);
                    res.emplace(std::move(str), std::move(file.second));
                }
            });
            return res;
        }
        case libbsa::file_format::fo4:
        {
            libbsa::fo4::archive arch;
            arch.read(std::move(path));
            btu::common::for_each_mt(std::move(arch), [&](auto &&elem) {
                res.emplace(elem.first.name(), std::move(elem.second));
            });
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
            btu::common::for_each_mt(std::move(arch), [&](Archive::value_type &&elem) {
                bsa.insert(elem.first, std::move(elem.second).template as_raw_file<libbsa::tes3::file>());
            });
            bsa.write(std::move(path));
            break;
        }
        case btu::bsa::ArchiveVersion::tes4: [[fallthrough]];
        case btu::bsa::ArchiveVersion::tes5: [[fallthrough]];
        case btu::bsa::ArchiveVersion::sse:
        {
            libbsa::tes4::archive bsa;
            btu::common::for_each_mt(std::move(arch), [&](Archive::value_type &&elem) {
                auto elem_path = Path(elem.first);
                const auto d   = [&]() {
                    const auto key = elem_path.parent_path().lexically_normal().generic_string();
                    if (bsa.find(key) == bsa.end())
                        bsa.insert(key, libbsa::tes4::directory());
                    return bsa[key];
                }();

                d->insert(elem_path.filename().lexically_normal().generic_string(),
                          std::move(elem.second).as_raw_file<libbsa::tes4::file>());
            });
            bsa.write(std::move(path), static_cast<libbsa::tes4::version>(version));
            break;
        }
        case btu::bsa::ArchiveVersion::fo4: [[fallthrough]];
        case btu::bsa::ArchiveVersion::fo4dx:
        {
            libbsa::fo4::archive ba2;
            btu::common::for_each_mt(std::move(arch), [&](Archive::value_type &&elem) {
                ba2.insert(elem.first, std::move(elem.second).template as_raw_file<libbsa::fo4::file>());
            });
            ba2.write(std::move(path), static_cast<libbsa::fo4::format>(version));
            break;
        }
    }
    libbsa::detail::declare_unreachable();
}

} // namespace btu::bsa
