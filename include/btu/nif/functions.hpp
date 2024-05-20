#pragma once

#include "btu/common/error.hpp"
#include "btu/common/games.hpp"
#include "btu/nif/mesh.hpp"

namespace btu::nif {
[[nodiscard]] auto convert(Mesh file, HeadpartStatus headpart, Game game) -> tl::expected<Mesh, Error>;
void rename_referenced_textures(Mesh &file);
} // namespace btu::nif
