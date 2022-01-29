#pragma once

#include "btu/common/error.hpp"
#include "btu/common/games.hpp"

#include <BasicTypes.hpp>

#include <optional>
#include <string>
#include <vector>

namespace btu::nif {
class Mesh;

struct Settings
{
    [[nodiscard]] static auto get(btu::common::Game game) noexcept -> const Settings &;

    btu::common::Game game;

    bool rename_referenced_textures;
    std::optional<nifly::NiVersion> target_version;
    std::vector<std::u8string> headpart_meshes; // This will be filled by the user, no need to fill it now
};

struct OptimizationSteps
{
    bool rename_referenced_textures;
    std::optional<btu::common::Game> format;
    bool headpart = false;

    auto operator<=>(const OptimizationSteps &) const noexcept = default;
};

auto optimize(Mesh &file, const OptimizationSteps &steps) -> btu::common::Error;
auto compute_optimization_steps(const Mesh &file, const Settings &sets) -> OptimizationSteps;
} // namespace btu::nif
