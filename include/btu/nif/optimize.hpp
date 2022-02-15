#pragma once

#include "btu/common/games.hpp"
#include "btu/nif/detail/common.hpp"

#include <nifly/BasicTypes.hpp>

#include <optional>
#include <string>
#include <vector>

namespace btu::nif {
class Mesh;

struct Settings
{
    [[nodiscard]] static auto get(btu::Game game) noexcept -> const Settings &;

    btu::Game game;

    bool rename_referenced_textures;
    std::optional<nifly::NiVersion> target_version;
    std::vector<std::u8string> headpart_meshes; // This will be filled by the user, no need to fill it now
};

struct OptimizationSteps
{
    bool rename_referenced_textures;
    std::optional<btu::Game> format;
    bool headpart = false;

    auto operator<=>(const OptimizationSteps &) const noexcept = default;
};

[[nodiscard]] auto optimize(Mesh file, const OptimizationSteps &steps) -> tl::expected<Mesh, Error>;
[[nodiscard]] auto compute_optimization_steps(const Mesh &file, const Settings &sets) -> OptimizationSteps;
} // namespace btu::nif
