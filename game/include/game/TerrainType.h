#pragma once

#include <cstdint>
#include <string_view>

namespace game {

enum class TerrainType : std::uint8_t {
    Plains,
    Hills,
    Forest,
    Water,
    Mountain,
};

[[nodiscard]] constexpr std::string_view terrainName(TerrainType terrain) {
    switch (terrain) {
    case TerrainType::Plains:
        return "Plains";
    case TerrainType::Hills:
        return "Hills";
    case TerrainType::Forest:
        return "Forest";
    case TerrainType::Water:
        return "Water";
    case TerrainType::Mountain:
        return "Mountain";
    }
    return "Unknown";
}

} // namespace game
