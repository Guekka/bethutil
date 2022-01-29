#pragma once

#include "btu/common/error.hpp"
#include "btu/common/games.hpp"
#include "btu/nif/mesh.hpp"

namespace btu::nif {
auto convert(Mesh &file, bool headpart, btu::common::Game game) -> bool;
auto rename_referenced_textures(Mesh &file) -> bool;
} // namespace btu::nif
