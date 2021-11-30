#pragma once

#include "btu/common/error.hpp"

#include <NifFile.hpp>
#include <tl/expected.hpp>

namespace btu::nif {
class Mesh
{
    nifly::NifFile file_;

public:
    auto get() -> nifly::NifFile & { return file_; }
    auto get() const -> const nifly::NifFile & { return file_; }
};

auto load(const std::filesystem::path &path) -> tl::expected<Mesh, btu::common::Error>;
auto save(const Mesh &file, const std::filesystem::path &path) -> btu::common::Error;
} // namespace btu::nif
