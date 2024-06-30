#pragma once

#include "btu/common/games.hpp"
#include "btu/nif/detail/common.hpp"

#include <btu/common/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace btu::nif {
class Mesh;

struct Settings
{
    [[nodiscard]] static auto get(Game game) noexcept -> const Settings &;

    // For SLE and SSE only
    Game target_game;
    std::vector<std::u8string> headpart_meshes;
    bool rename_referenced_textures = false;

    bool optimize = false;
};

// NOTE: we ignore headpart_meshes on purpose. While this could result in a loss of data, it is a compromise
// to reduce the amount of data that needs to be stored in the settings file. This list tends to be very long
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Settings, target_game, rename_referenced_textures)

struct OptimizationSteps
{
    bool optimize = false;

    bool rename_referenced_textures = true;
    std::optional<Game> format;
    HeadpartStatus headpart = HeadpartStatus::No;

    auto operator<=>(const OptimizationSteps &) const noexcept = default;
};

[[nodiscard]] auto optimize(Mesh file, const OptimizationSteps &steps) -> tl::expected<Mesh, Error>;
[[nodiscard]] auto compute_optimization_steps(const Mesh &file, const Settings &sets) -> OptimizationSteps;
} // namespace btu::nif
