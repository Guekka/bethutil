#pragma once

#include "btu/common/error.hpp"
#include "btu/common/path.hpp"

#include <NifFile.hpp>
#include <tl/expected.hpp>

namespace btu::nif {
using btu::common::Path;
using Error       = btu::common::Error;
using ResultError = tl::expected<std::monostate, Error>;

class Mesh
{
    nifly::NifFile file_;
    Path load_path_;

public:
    [[nodiscard]] auto load(Path path) -> ResultError;
    [[nodiscard]] auto save(const Path &path) -> ResultError;

    auto get() noexcept -> nifly::NifFile &;
    auto get() const noexcept -> const nifly::NifFile &;

    auto get_load_path() const noexcept -> const Path &;
    void set_load_path(Path path) noexcept;
};

auto canonize_path(std::filesystem::path path) noexcept -> std::u8string;

} // namespace btu::nif
