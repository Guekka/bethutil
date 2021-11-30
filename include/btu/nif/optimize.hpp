#pragma once

#include "btu/common/error.hpp"
#include "btu/common/games.hpp"

#include <NifFile.hpp>

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
    std::vector<std::u8string> landscape_textures; // This will be filled by the user, no need to fill it now
};

struct OptimizationSteps
{
    bool rename_referenced_textures;
    std::optional<btu::common::Game> format;

    auto operator<=>(const OptimizationSteps &) const noexcept = default;
};

auto optimize(Mesh &file, const Settings &sets) -> btu::common::Error;
auto compute_optimization_steps(const Mesh &file, const OptimizationSteps &steps) -> Settings;
} // namespace btu::nif
