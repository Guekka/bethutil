#pragma once

#include "btu/bsa/detail/archive_type.hpp"

#include <functional>

namespace btu::bsa::detail {

class Archive
{
public:
    virtual ArchiveVersion read(const std::filesystem::path &a_path) = 0;
    virtual void write(std::filesystem::path a_path)                 = 0;

    virtual size_t add_file(const std::filesystem::path &a_root, const std::filesystem::path &a_path) = 0;
    virtual size_t add_file(const std::filesystem::path &a_relative, std::vector<std::byte> a_data)   = 0;

    using iteration_callback = std::function<void(const std::filesystem::path &, std::span<const std::byte>)>;
    virtual void iterate_files(const iteration_callback &a_callback, bool skip_compressed = false) = 0;

    virtual ArchiveVersion get_version() const noexcept = 0;
};

} // namespace btu::bsa::detail
