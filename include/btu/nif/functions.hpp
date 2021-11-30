#pragma once

#include "btu/common/error.hpp"
#include "btu/common/games.hpp"
#include "btu/nif/mesh.hpp"

namespace btu::nif {
auto convert(Mesh &file, btu::common::Game game) -> void;
auto rename_referenced_textures(Mesh &file) -> void;
} // namespace btu::nif
