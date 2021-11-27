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

    virtual auto unpack(const Path &out_path) -> void = 0;

    [[nodiscard]] virtual auto get_version() const noexcept -> ArchiveVersion = 0;
};

} // namespace btu::bsa::detail
