#include "btu/nif/optimize.hpp"

namespace btu::nif {
auto optimize(Mesh &file, const Settings &sets) -> btu::common::Error
{
    // Process the file according to settings. Use `convert` and `rename_referenced_textures`
}

auto compute_optimization_steps(const Mesh &file, const OptimizationSteps &steps) -> Settings {}
} // namespace btu::nif
