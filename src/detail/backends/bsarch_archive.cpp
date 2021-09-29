#include "btu/bsa/detail/backends/bsarch_archive.hpp"

#include "btu/bsa/detail/common.hpp"

#include <execution>
#include <fstream>

namespace btu::bsa::detail {

using map_type = std::pair<ArchiveVersion, bsa_archive_type_t>;

auto version_pair = std::vector<map_type>{
    {ArchiveVersion::tes3, bsa_archive_type_t::baTES3},
    {ArchiveVersion::tes4, bsa_archive_type_t::baTES4},
    {ArchiveVersion::fo3, bsa_archive_type_t::baFO3},
    {ArchiveVersion::sse, bsa_archive_type_t::baSSE},
    {ArchiveVersion::fo4, bsa_archive_type_t::baFO4},
    {ArchiveVersion::fo4dx, bsa_archive_type_t::baFO4dds},
};

bsa_archive_type_t to_libbsarch_version(ArchiveVersion version)
{
    return std::ranges::find(version_pair, version, &map_type::first)->second;
}

ArchiveVersion to_version(bsa_archive_type_t version)
{
    return std::ranges::find(version_pair, version, &map_type::second)->first;
}

BsarchArchive::BsarchArchive(const std::filesystem::path &a_path)
{
    read(a_path);
}

BsarchArchive::BsarchArchive(ArchiveVersion a_version, bool a_compressed)
    : _version(to_libbsarch_version(a_version))
    , _compressed(a_compressed)
{
}

auto BsarchArchive::read(const std::filesystem::path &a_path) -> ArchiveVersion
{
    _archive.load(a_path);
    return to_version(_archive.get_type());
}

void BsarchArchive::write(std::filesystem::path a_path)
{
    libbsarch::bsa_entry_list file_list;

    for (const auto &file : files_)
        file_list.add(file.relative);

    libbsarch::bsa_saver_complex saver(std::move(_archive));
    saver.prepare(a_path, std::move(file_list), _version);

    for (auto &&file : std::move(files_))
    {
        const auto &relative = file.relative;
        std::visit(btu::common::overload{
                       [&](std::filesystem::path &&path) {
                           auto blob = libbsarch::disk_blob(relative,
                                                            path,
                                                            libbsarch::from_path_in_archive_tag{});
                           saver.add_file(std::move(blob));
                       },
                       [&](std::vector<std::byte> &&data) { saver.add_file(relative, std::move(data)); },
                   },
                   std::move(file.full));
    }
}

size_t BsarchArchive::add_file(const std::filesystem::path &a_root, const std::filesystem::path &a_path)
{
    const auto relative = a_path.lexically_relative(a_root).lexically_normal();
    files_.emplace_back(File{relative, a_path});
    return std::filesystem::file_size(a_path);
}

size_t BsarchArchive::add_file(const std::filesystem::path &a_relative, std::vector<std::byte> a_data)
{
    files_.emplace_back(File{a_relative, a_data});
    return a_data.size();
}

void BsarchArchive::iterate_files(const iteration_callback &a_callback, bool)
{
    auto callback = a_callback;

    _archive.iterate_files(
        [](bsa_archive_t archive,
           const wchar_t *file_path,
           bsa_file_record_t file_record,
           bsa_folder_record_t,
           void *context) {
            auto data = bsa_extract_file_data_by_record(archive, file_record);
            auto span = std::span(static_cast<std::byte *>(data.buffer.data), data.buffer.size);

            auto callback = *static_cast<iteration_callback *>(context);
            callback(std::filesystem::path(file_path), span);

            return false;
        },
        &callback);
}

ArchiveVersion BsarchArchive::get_version() const noexcept
{
    return to_version(_version);
}

const libbsarch::bsa &BsarchArchive::get_archive() const noexcept
{
    return _archive;
}

} // namespace btu::bsa::detail
