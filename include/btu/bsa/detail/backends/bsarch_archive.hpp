#pragma once

#include "btu/bsa/detail/backends/archive.hpp"
#include "libbsarch-cpp/bsa_saver.hpp"

#include <functional>
#include <mutex>
#include <variant>

namespace btu::bsa::detail {

bsa_archive_type_t to_libbsarch_version(ArchiveVersion version);
ArchiveVersion to_version(bsa_archive_type_t version);

class BsarchArchive : public Archive
{
public:
    BsarchArchive(const std::filesystem::path &a_path); // Read
    BsarchArchive(ArchiveVersion a_version, bool a_compressed);

    ArchiveVersion read(const std::filesystem::path &a_path) override;

    /// Consumes the archive
    void write(std::filesystem::path a_path) override;

    void add_file(const std::filesystem::path &a_root, const std::filesystem::path &a_path) override;
    void add_file(const std::filesystem::path &a_relative, std::vector<std::byte> a_data) override;

    using iteration_callback = std::function<void(const std::filesystem::path &, std::span<const std::byte>)>;
    void iterate_files(const iteration_callback &a_callback, bool skip_compressed = false) override;

    ArchiveVersion get_version() const noexcept override;
    const libbsarch::bsa &get_archive() const noexcept;

private:
    struct File
    {
        std::filesystem::path relative;
        std::variant<std::filesystem::path, std::vector<std::byte>> full;
    };
    std::vector<File> files_;

    libbsarch::bsa _archive;

    bsa_archive_type_t _version;
    bool _compressed;
};

} // namespace btu::bsa::detail
