#include "btu/nif/optimize.hpp"

#include "btu/common/algorithms.hpp"
#include "btu/nif/functions.hpp"
#include "btu/nif/mesh.hpp"

namespace btu::nif {
auto optimize(Mesh file, const OptimizationSteps &sets) -> tl::expected<Mesh, Error>
{
    auto res = tl::expected<Mesh, Error>(std::move(file));
    if (sets.rename_referenced_textures)
        res.map([](auto &m) { rename_referenced_textures(m); });
    if (sets.format)
        if (res = convert(std::move(res).value(), sets.headpart, *sets.format); !res)
            return res;
    return res;
}

auto compute_optimization_steps(const Mesh &file, const Settings &sets) -> OptimizationSteps
{
    const auto path        = canonize_path(file.get_load_path());
    const bool is_headpart = btu::common::contains(sets.headpart_meshes, path);

    // We only convert SSE and LE
    std::optional<btu::Game> format{};
    
    const bool sse = sets.game == btu::Game::SSE && !file.get().IsSSECompatible();
    const bool sle = sets.game == btu::Game::SLE;
    if (sse || sle)
        format = sets.game;

    return OptimizationSteps{
        .rename_referenced_textures = sets.rename_referenced_textures,
        .format                     = format,
        .headpart                   = is_headpart,
    };
}
} // namespace btu::nif
