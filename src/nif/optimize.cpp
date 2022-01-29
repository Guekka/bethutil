#include "btu/nif/optimize.hpp"

#include "btu/common/algorithms.hpp"
#include "btu/nif/functions.hpp"
#include "btu/nif/mesh.hpp"

namespace btu::nif {
auto optimize(Mesh &file, const OptimizationSteps &sets) -> btu::common::Error
{
    if (sets.rename_referenced_textures)
        rename_referenced_textures(file);
    if (sets.format)
        if (!convert(file, sets.headpart, *sets.format))
            return btu::common::Error(std::error_code(1, std::generic_category()));
    return btu::common::Error::success();
}

auto compute_optimization_steps(const Mesh &file, const Settings &sets) -> OptimizationSteps
{
    const auto path        = canonize_path(file.get_load_path());
    const bool is_headpart = btu::common::contains(sets.headpart_meshes, path);

    // We only convert SSE and LE
    std::optional<btu::common::Game> format{};
    if (sets.game == btu::common::Game::SSE && !file.get().IsSSECompatible())
        format = sets.game;
    else if (sets.game == btu::common::Game::SLE)
        format = sets.game;

    return OptimizationSteps{
        .rename_referenced_textures = sets.rename_referenced_textures,
        .format                     = format,
        .headpart                   = is_headpart,
    };
}
} // namespace btu::nif
