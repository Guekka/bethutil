#pragma once

#include "btu/bsa/detail/archive_type.hpp"
#include "btu/bsa/detail/common.hpp"

#include <filesystem>
#include <functional>
#include <span>

namespace btu::bsa::detail {

class Archive
{
public:
    virtual auto read(Path a_path) -> ArchiveVersion = 0;
    virtual void write(Path a_path)                  = 0;

    virtual void add_file(const Path &a_root, const Path &a_path)                = 0;
    virtual void add_file(const Path &a_relative, std::vector<std::byte> a_data) = 0;

    using iteration_callback = std::function<void(const Path &, std::span<const std::byte>)>;
    virtual void iterate_files(const iteration_callback &a_callback, bool skip_compressed = false) = 0;

    [[nodiscard]] virtual auto get_version() const noexcept -> ArchiveVersion = 0;
};

} // namespace btu::bsa::detail
