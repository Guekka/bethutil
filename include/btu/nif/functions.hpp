#pragma once

#include "btu/common/error.hpp"
#include "btu/common/games.hpp"
#include "btu/nif/mesh.hpp"

namespace btu::nif {
enum class HeadpartStatus
{
    Yes,
    No,
};

[[nodiscard]] auto convert(Mesh file, HeadpartStatus headpart, btu::Game game) -> tl::expected<Mesh, Error>;
void rename_referenced_textures(Mesh &file);
} // namespace btu::nif
