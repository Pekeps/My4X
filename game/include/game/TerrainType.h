#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace game {

enum class TerrainType : std::uint8_t {
    Plains,
    Hills,
    Forest,
    Water,
    Mountain,
    Desert,
    Swamp,
};

/// Total number of terrain types in the enum.
static constexpr std::size_t TERRAIN_TYPE_COUNT = 7;

/// Properties that define gameplay behavior for each terrain type.
struct TerrainProperties {
    /// Movement cost to enter this tile (0 = impassable).
    int movementCost;
    /// Defense modifier applied to units standing on this tile.
    int defenseModifier;
    /// Whether land units can traverse this tile at all.
    bool passable;
    /// Whether buildings can be placed on this tile.
    bool buildable;
};

// ── Named constants for terrain property values ─────────────────────────────

namespace terrain_constants {

static constexpr int PLAINS_MOVEMENT_COST = 1;
static constexpr int PLAINS_DEFENSE_MODIFIER = 0;

static constexpr int HILLS_MOVEMENT_COST = 2;
static constexpr int HILLS_DEFENSE_MODIFIER = 3;

static constexpr int FOREST_MOVEMENT_COST = 2;
static constexpr int FOREST_DEFENSE_MODIFIER = 2;

static constexpr int WATER_MOVEMENT_COST = 0;
static constexpr int WATER_DEFENSE_MODIFIER = 0;

static constexpr int MOUNTAIN_MOVEMENT_COST = 0;
static constexpr int MOUNTAIN_DEFENSE_MODIFIER = 0;

static constexpr int DESERT_MOVEMENT_COST = 1;
static constexpr int DESERT_DEFENSE_MODIFIER = 0;

static constexpr int SWAMP_MOVEMENT_COST = 3;
static constexpr int SWAMP_DEFENSE_MODIFIER = -1;

} // namespace terrain_constants

/// Compile-time table of terrain properties, indexed by TerrainType.
inline constexpr std::array<TerrainProperties, TERRAIN_TYPE_COUNT> TERRAIN_PROPERTIES = {{
    // Plains: default terrain, easy to traverse, no bonuses
    {.movementCost = terrain_constants::PLAINS_MOVEMENT_COST,
     .defenseModifier = terrain_constants::PLAINS_DEFENSE_MODIFIER,
     .passable = true,
     .buildable = true},
    // Hills: slower movement, good defensive position, mining buildings
    {.movementCost = terrain_constants::HILLS_MOVEMENT_COST,
     .defenseModifier = terrain_constants::HILLS_DEFENSE_MODIFIER,
     .passable = true,
     .buildable = true},
    // Forest: slower movement, moderate cover, lumber-related buildings
    {.movementCost = terrain_constants::FOREST_MOVEMENT_COST,
     .defenseModifier = terrain_constants::FOREST_DEFENSE_MODIFIER,
     .passable = true,
     .buildable = true},
    // Water: impassable to land units, no land buildings
    {.movementCost = terrain_constants::WATER_MOVEMENT_COST,
     .defenseModifier = terrain_constants::WATER_DEFENSE_MODIFIER,
     .passable = false,
     .buildable = false},
    // Mountain: impassable, no buildings
    {.movementCost = terrain_constants::MOUNTAIN_MOVEMENT_COST,
     .defenseModifier = terrain_constants::MOUNTAIN_DEFENSE_MODIFIER,
     .passable = false,
     .buildable = false},
    // Desert: fast movement, no defense bonus, reduced food yield
    {.movementCost = terrain_constants::DESERT_MOVEMENT_COST,
     .defenseModifier = terrain_constants::DESERT_DEFENSE_MODIFIER,
     .passable = true,
     .buildable = true},
    // Swamp: very slow movement, defense penalty, limited buildings
    {.movementCost = terrain_constants::SWAMP_MOVEMENT_COST,
     .defenseModifier = terrain_constants::SWAMP_DEFENSE_MODIFIER,
     .passable = true,
     .buildable = true},
}};

/// Look up the gameplay properties for a terrain type.
/// Returns a const reference to the compile-time property table entry.
[[nodiscard]] constexpr const TerrainProperties &getTerrainProperties(TerrainType terrain) {
    return TERRAIN_PROPERTIES.at(static_cast<std::size_t>(terrain));
}

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
    case TerrainType::Desert:
        return "Desert";
    case TerrainType::Swamp:
        return "Swamp";
    }
    return "Unknown";
}

} // namespace game
