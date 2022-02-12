#pragma once

#include "btu/common/error.hpp"
#include "btu/common/games.hpp"
#include "btu/nif/mesh.hpp"

namespace btu::nif {
[[nodiscard]] auto convert(Mesh &file, bool headpart, btu::common::Game game) -> ResultError;
void rename_referenced_textures(Mesh &file);
} // namespace btu::nif
