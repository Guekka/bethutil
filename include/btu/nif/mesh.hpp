#pragma once

#include "btu/common/error.hpp"
#include "btu/common/path.hpp"
#include "btu/nif/common.hpp"

#include <nifly/NifFile.hpp>
#include <tl/expected.hpp>

namespace btu::nif {
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

[[maybe_unused]] constexpr auto canonize_path = common::make_path_canonizer(u8"meshes/");

} // namespace btu::nif
