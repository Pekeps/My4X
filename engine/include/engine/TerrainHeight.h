#pragma once

#include "game/TerrainType.h"

#include <array>
#include <numbers>

namespace engine::terrain_height {

// ── Per-terrain Y-axis elevation offsets ─────────────────────────────────────
// Applied to hex tile centers and vertices so each terrain type sits at a
// distinct height on the XZ plane.

static constexpr float PLAINS_HEIGHT = 0.0F;
static constexpr float FOREST_HEIGHT = 0.0F;
static constexpr float HILLS_HEIGHT = 0.3F;
static constexpr float MOUNTAIN_HEIGHT = 0.8F;
static constexpr float WATER_HEIGHT = -0.1F;
static constexpr float DESERT_HEIGHT = 0.0F;
static constexpr float SWAMP_HEIGHT = -0.05F;

// ── Lookup table indexed by TerrainType ──────────────────────────────────────

static constexpr std::array<float, game::TERRAIN_TYPE_COUNT> TERRAIN_HEIGHTS = {{
    PLAINS_HEIGHT,   // Plains
    HILLS_HEIGHT,    // Hills
    FOREST_HEIGHT,   // Forest
    WATER_HEIGHT,    // Water
    MOUNTAIN_HEIGHT, // Mountain
    DESERT_HEIGHT,   // Desert
    SWAMP_HEIGHT,    // Swamp
}};

/// Return the Y-axis height offset for a terrain type.
[[nodiscard]] constexpr float terrainHeight(game::TerrainType terrain) {
    return TERRAIN_HEIGHTS.at(static_cast<std::size_t>(terrain));
}

// ── Decoration geometry constants ────────────────────────────────────────────

// Forest tree decoration dimensions
static constexpr float TREE_TRUNK_RADIUS = 0.03F;
static constexpr float TREE_TRUNK_HEIGHT = 0.15F;
static constexpr float TREE_CANOPY_RADIUS = 0.1F;
static constexpr float TREE_CANOPY_HEIGHT = 0.2F;

/// Number of trees placed per forest hex.
static constexpr int FOREST_TREE_COUNT = 5;

/// Radial distance from hex center for tree placement ring.
static constexpr float TREE_RING_RADIUS = 0.45F;

/// Angular offset (degrees) between successive trees in the ring.
static constexpr float TREE_ANGLE_STEP = 72.0F;

/// Starting angle (degrees) for tree placement ring.
static constexpr float TREE_ANGLE_START = 15.0F;

// Mountain peak decoration dimensions
static constexpr float MOUNTAIN_PEAK_RADIUS = 0.35F;
static constexpr float MOUNTAIN_PEAK_HEIGHT = 0.6F;

// Mountain peak (snow cap) proportions
static constexpr float SNOW_CAP_RADIUS = 0.15F;
static constexpr float SNOW_CAP_HEIGHT = 0.2F;

// ── Decoration colors ────────────────────────────────────────────────────────

static constexpr Color TREE_TRUNK_COLOR = {.r = 100, .g = 70, .b = 30, .a = 255};
static constexpr Color TREE_CANOPY_COLOR = {.r = 30, .g = 100, .b = 30, .a = 255};
static constexpr Color MOUNTAIN_PEAK_COLOR = {.r = 130, .g = 130, .b = 135, .a = 255};
static constexpr Color MOUNTAIN_SNOW_COLOR = {.r = 240, .g = 240, .b = 250, .a = 255};

// Hills bump decoration
static constexpr float HILLS_BUMP_RADIUS = 0.25F;

// Number of slices/rings for sphere approximation
static constexpr int SPHERE_SLICES = 8;
static constexpr int SPHERE_RINGS = 8;

// Cone rendering slices
static constexpr int CONE_SLICES = 8;

// Cylinder rendering slices
static constexpr int CYLINDER_SLICES = 6;

// Degrees-to-radians factor for tree placement math
static constexpr float DEG_TO_RAD_FACTOR = std::numbers::pi_v<float> / 180.0F;

} // namespace engine::terrain_height
