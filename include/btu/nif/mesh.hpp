#pragma once

#include "btu/common/error.hpp"
#include "btu/common/path.hpp"
#include "btu/nif/detail/common.hpp"

#include <nifly/NifFile.hpp>
#include <tl/expected.hpp>

namespace btu::nif {
class Mesh
{
    nifly::NifFile file_;
    Path load_path_;

public:
    auto get() noexcept -> nifly::NifFile &;
    auto get() const noexcept -> const nifly::NifFile &;

    auto get_load_path() const noexcept -> const Path &;
    void set_load_path(Path path) noexcept;
};

[[maybe_unused]] constexpr auto canonize_path = common::make_path_canonizer(u8"meshes/");

[[nodiscard]] auto load(Path path) noexcept -> tl::expected<Mesh, Error>;
[[nodiscard]] auto save(Mesh mesh, const Path &path) noexcept -> ResultError;

} // namespace btu::nif
