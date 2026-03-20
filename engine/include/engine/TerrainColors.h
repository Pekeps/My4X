#pragma once

#include "game/TerrainType.h"

#include "raylib.h"

#include <array>

namespace engine::terrain_colors {

// ── Terrain fill colors ─────────────────────────────────────────────────────
// Each terrain type maps to a distinct fill color for hex tiles.

static constexpr Color PLAINS_FILL = {.r = 140, .g = 200, .b = 90, .a = 255};
static constexpr Color HILLS_FILL = {.r = 190, .g = 160, .b = 110, .a = 255};
static constexpr Color FOREST_FILL = {.r = 50, .g = 120, .b = 50, .a = 255};
static constexpr Color WATER_FILL = {.r = 60, .g = 120, .b = 200, .a = 255};
static constexpr Color MOUNTAIN_FILL = {.r = 150, .g = 150, .b = 155, .a = 255};
static constexpr Color DESERT_FILL = {.r = 220, .g = 200, .b = 130, .a = 255};
static constexpr Color SWAMP_FILL = {.r = 80, .g = 100, .b = 55, .a = 255};

// ── Terrain outline colors (slightly darker than fill) ──────────────────────

static constexpr Color PLAINS_OUTLINE = {.r = 100, .g = 160, .b = 60, .a = 255};
static constexpr Color HILLS_OUTLINE = {.r = 150, .g = 120, .b = 80, .a = 255};
static constexpr Color FOREST_OUTLINE = {.r = 30, .g = 80, .b = 30, .a = 255};
static constexpr Color WATER_OUTLINE = {.r = 40, .g = 80, .b = 160, .a = 255};
static constexpr Color MOUNTAIN_OUTLINE = {.r = 100, .g = 100, .b = 110, .a = 255};
static constexpr Color DESERT_OUTLINE = {.r = 180, .g = 160, .b = 90, .a = 255};
static constexpr Color SWAMP_OUTLINE = {.r = 50, .g = 70, .b = 35, .a = 255};

// ── Lookup tables indexed by TerrainType ────────────────────────────────────

static constexpr std::array<Color, game::TERRAIN_TYPE_COUNT> TERRAIN_FILL_COLORS = {{
    PLAINS_FILL,   // Plains
    HILLS_FILL,    // Hills
    FOREST_FILL,   // Forest
    WATER_FILL,    // Water
    MOUNTAIN_FILL, // Mountain
    DESERT_FILL,   // Desert
    SWAMP_FILL,    // Swamp
}};

static constexpr std::array<Color, game::TERRAIN_TYPE_COUNT> TERRAIN_OUTLINE_COLORS = {{
    PLAINS_OUTLINE,   // Plains
    HILLS_OUTLINE,    // Hills
    FOREST_OUTLINE,   // Forest
    WATER_OUTLINE,    // Water
    MOUNTAIN_OUTLINE, // Mountain
    DESERT_OUTLINE,   // Desert
    SWAMP_OUTLINE,    // Swamp
}};

/// Return the fill color for a terrain type.
[[nodiscard]] inline Color terrainFillColor(game::TerrainType terrain) {
    return TERRAIN_FILL_COLORS.at(static_cast<std::size_t>(terrain));
}

/// Return the outline color for a terrain type.
[[nodiscard]] inline Color terrainOutlineColor(game::TerrainType terrain) {
    return TERRAIN_OUTLINE_COLORS.at(static_cast<std::size_t>(terrain));
}

} // namespace engine::terrain_colors
