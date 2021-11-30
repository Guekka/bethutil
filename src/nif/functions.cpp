#include "btu/nif/functions.hpp"

namespace btu::nif {
auto convert(Mesh &file, bool convert, common::Game game) -> void
{
    // See https://gitlab.com/G_ka/Cathedral_Assets_Optimizer/-/blob/dev/src/Commands/Meshes/MeshConvert.cpp
}

auto rename_referenced_textures(Mesh &file) -> void
{
    // See https://gitlab.com/G_ka/Cathedral_Assets_Optimizer/-/blob/master/src/MeshesOptimizer.cpp#L152
}
} // namespace btu::nif
