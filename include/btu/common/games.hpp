#pragma once

#include <btu/common/json.hpp>

namespace btu {
enum class Game : std::uint8_t
{
    TES3,
    TES4,
    FNV,
    SLE,
    SSE,
    FO4,
    Custom
};

NLOHMANN_JSON_SERIALIZE_ENUM(Game,
                             {{Game::TES3, "tes3"},
                              {Game::TES4, "tes4"},
                              {Game::FNV, "fnv"},
                              {Game::SLE, "sle"},
                              {Game::SSE, "sse"},
                              {Game::FO4, "fo4"},
                              {Game::Custom, "custom"}})

} // namespace btu
