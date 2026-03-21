#include "engine/MapRenderer.h"

#include "HexDraw.h"
#include "engine/EdgeVertices.h"
#include "engine/HexGrid.h"
#include "engine/HexMeshBuilder.h"
#include "engine/HexMetrics.h"
#include "engine/TerrainColors.h"
#include "engine/TerrainHeight.h"
#include "game/HexDirection.h"

#include "raylib.h"

#include <array>
#include <cmath>
#include <numbers>

namespace engine {

/// Color used to render unexplored (never-seen) tiles.
static constexpr Color FOG_UNEXPLORED_COLOR = {.r = 10, .g = 10, .b = 15, .a = 255};

/// Semi-transparent dark overlay drawn on explored-but-not-visible tiles
/// to dim them to roughly 50% brightness.
static constexpr Color FOG_EXPLORED_OVERLAY = {.r = 0, .g = 0, .b = 0, .a = 128};

/// Outline color for unexplored tiles (slightly lighter than fill for shape).
static constexpr Color FOG_UNEXPLORED_OUTLINE = {.r = 20, .g = 20, .b = 30, .a = 255};

/// Dim a color by blending it toward black (simulates the explored overlay).
/// factor 0 = full black, 1 = original color.
static constexpr float EXPLORED_DIM_FACTOR = 0.5F;

/// Half height used for trunk cylinder positioning.
static constexpr float TRUNK_HALF_DIVISOR = 2.0F;

/// Starting angle for hex corner computation (degrees).
static constexpr float HEX_START_ANGLE = 30.0F;

/// Degrees between consecutive hex vertices.
static constexpr float HEX_DEGREES_PER_SIDE = 60.0F;

/// Conversion factor from degrees to radians.
static constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0F;

/// Number of hex sides / vertices.
static constexpr int HEX_SIDES = 6;

/// Number of "near" edge directions processed for blending (NE, E, SE).
static constexpr int NEAR_DIRECTION_COUNT = 3;

/// Color blend factor for corner triangles: average of 3 colors uses 1/3 weight.
static constexpr float CORNER_BLEND_THIRD = 1.0F / 3.0F;

/// Color blend factor for corner triangles: 2/3 weight.
static constexpr float CORNER_BLEND_TWO_THIRDS = 2.0F / 3.0F;

static Color dimColor(Color base) {
    return Color{
        .r = static_cast<unsigned char>(static_cast<float>(base.r) * EXPLORED_DIM_FACTOR),
        .g = static_cast<unsigned char>(static_cast<float>(base.g) * EXPLORED_DIM_FACTOR),
        .b = static_cast<unsigned char>(static_cast<float>(base.b) * EXPLORED_DIM_FACTOR),
        .a = base.a,
    };
}

/// Compute hex corners at a given radius around a center point.
static std::array<Vector3, HEX_SIDES> hexCornersAtRadius(Vector3 center, float radius) {
    std::array<Vector3, HEX_SIDES> corners{};
    for (int i = 0; i < HEX_SIDES; ++i) {
        float angle = (HEX_START_ANGLE + (static_cast<float>(i) * HEX_DEGREES_PER_SIDE)) * DEG_TO_RAD;
        corners.at(i) = {
            .x = center.x + (radius * cosf(angle)),
            .y = center.y,
            .z = center.z + (radius * sinf(angle)),
        };
    }
    return corners;
}

/// Check if a neighbor in a given direction is visible (not unexplored).
static bool isNeighborVisible(const game::Map &map, int row, int col, game::HexDirection dir, const game::FogOfWar *fog,
                              game::FactionId playerId) {
    auto neighbor = game::neighborCoord(row, col, dir, map.height(), map.width());
    if (!neighbor) {
        return false;
    }
    auto [nr, nc] = *neighbor;
    if (fog == nullptr) {
        return true;
    }
    return fog->getVisibility(playerId, nr, nc) != game::VisibilityState::Unexplored;
}

/// Get the terrain fill color of a neighbor cell. Returns ownColor for
/// out-of-bounds or unexplored neighbors.
static Color getNeighborColor(const game::Map &map, int row, int col, game::HexDirection dir, Color ownColor,
                              const game::FogOfWar *fog, game::FactionId playerId, game::VisibilityState ownVis) {
    auto neighbor = game::neighborCoord(row, col, dir, map.height(), map.width());
    if (!neighbor) {
        return ownColor;
    }
    auto [nr, nc] = *neighbor;
    if (fog != nullptr && fog->getVisibility(playerId, nr, nc) == game::VisibilityState::Unexplored) {
        return ownColor;
    }
    Color nColor = terrain_colors::terrainFillColor(map.tile(nr, nc).terrainType());
    // If current cell is explored (dimmed), dim the neighbor color too for consistency.
    if (ownVis == game::VisibilityState::Explored) {
        nColor = dimColor(nColor);
    }
    return nColor;
}

/// Compute blended corner color from this cell and its two adjacent neighbors.
static Color cornerBlendColor(const game::Map &map, int row, int col, int dirIdx, Color cellColor,
                              const game::FogOfWar *fog, game::FactionId playerId, game::VisibilityState vis) {
    auto dir = game::ALL_DIRECTIONS.at(dirIdx);
    int nextD = (dirIdx + 1) % HEX_SIDES;
    auto nextDir = game::ALL_DIRECTIONS.at(nextD);

    Color color1 = getNeighborColor(map, row, col, dir, cellColor, fog, playerId, vis);
    Color color2 = getNeighborColor(map, row, col, nextDir, cellColor, fog, playerId, vis);

    return lerpColor(cellColor, lerpColor(color1, color2, CORNER_BLEND_THIRD), CORNER_BLEND_TWO_THIRDS);
}

/// Add the inner solid hex (6 triangles from center to inner corners).
static void addInnerHex(HexMeshBuilder &builder, Vector3 center, const std::array<Vector3, HEX_SIDES> &innerCorners,
                        Color cellColor) {
    for (int i = 0; i < HEX_SIDES; ++i) {
        int next = (i + 1) % HEX_SIDES;
        builder.addTriangle(center, innerCorners.at(next), innerCorners.at(i), cellColor);
    }
}

/// Add edge blend quads for the first 3 directions (NE, E, SE).
static void addNearEdges(HexMeshBuilder &builder, const game::Map &map, int row, int col,
                         const std::array<Vector3, HEX_SIDES> &innerCorners,
                         const std::array<Vector3, HEX_SIDES> &outerCorners, Color cellColor, const game::FogOfWar *fog,
                         game::FactionId playerId, game::VisibilityState vis) {
    for (int d = 0; d < NEAR_DIRECTION_COUNT; ++d) {
        auto dir = game::ALL_DIRECTIONS.at(d);
        int vi = d;
        int vn = (d + 1) % HEX_SIDES;

        Color neighborColor = getNeighborColor(map, row, col, dir, cellColor, fog, playerId, vis);

        builder.addQuad(innerCorners.at(vi), innerCorners.at(vn), outerCorners.at(vn), outerCorners.at(vi), cellColor,
                        cellColor, neighborColor, neighborColor);
    }
}

/// Add edge blend quads for the last 3 directions (SW, W, NW) where no
/// visible neighbor exists to handle the opposite side.
static void addFarEdges(HexMeshBuilder &builder, const game::Map &map, int row, int col,
                        const std::array<Vector3, HEX_SIDES> &innerCorners,
                        const std::array<Vector3, HEX_SIDES> &outerCorners, Color cellColor, const game::FogOfWar *fog,
                        game::FactionId playerId) {
    for (int d = NEAR_DIRECTION_COUNT; d < HEX_SIDES; ++d) {
        auto dir = game::ALL_DIRECTIONS.at(d);
        bool hasVisibleNeighbor = isNeighborVisible(map, row, col, dir, fog, playerId);

        if (!hasVisibleNeighbor) {
            int vi = d;
            int vn = (d + 1) % HEX_SIDES;
            builder.addQuad(innerCorners.at(vi), innerCorners.at(vn), outerCorners.at(vn), outerCorners.at(vi),
                            cellColor, cellColor, cellColor, cellColor);
        }
    }
}

/// Add corner blend triangles for the first 3 directions (NE, E, SE).
static void addNearCorners(HexMeshBuilder &builder, const game::Map &map, int row, int col,
                           const std::array<Vector3, HEX_SIDES> &innerCorners,
                           const std::array<Vector3, HEX_SIDES> &outerCorners, Color cellColor,
                           const game::FogOfWar *fog, game::FactionId playerId, game::VisibilityState vis) {
    for (int d = 0; d < NEAR_DIRECTION_COUNT; ++d) {
        int cornerIdx = (d + 1) % HEX_SIDES;
        Color blended = cornerBlendColor(map, row, col, d, cellColor, fog, playerId, vis);
        builder.addTriangle(innerCorners.at(cornerIdx), outerCorners.at(cornerIdx), outerCorners.at(cornerIdx),
                            blended);
    }
}

/// Add corner blend triangles for the last 3 directions (SW, W, NW) where
/// at least one adjacent neighbor is missing or unexplored.
static void addFarCorners(HexMeshBuilder &builder, const game::Map &map, int row, int col,
                          const std::array<Vector3, HEX_SIDES> &innerCorners,
                          const std::array<Vector3, HEX_SIDES> &outerCorners, Color cellColor,
                          const game::FogOfWar *fog, game::FactionId playerId, game::VisibilityState vis) {
    for (int d = NEAR_DIRECTION_COUNT; d < HEX_SIDES; ++d) {
        auto dir = game::ALL_DIRECTIONS.at(d);
        int nextD = (d + 1) % HEX_SIDES;
        auto nextDir = game::ALL_DIRECTIONS.at(nextD);

        bool n1Visible = isNeighborVisible(map, row, col, dir, fog, playerId);
        bool n2Visible = isNeighborVisible(map, row, col, nextDir, fog, playerId);

        if (!n1Visible || !n2Visible) {
            int cornerIdx = (d + 1) % HEX_SIDES;
            Color blended = cornerBlendColor(map, row, col, d, cellColor, fog, playerId, vis);
            builder.addTriangle(innerCorners.at(cornerIdx), outerCorners.at(cornerIdx), outerCorners.at(cornerIdx),
                                blended);
        }
    }
}

/// Draw small cone/sphere "trees" on a forest hex tile.
static void drawForestTrees(Vector3 center) {
    namespace th = terrain_height;
    for (int i = 0; i < th::FOREST_TREE_COUNT; ++i) {
        float angle = (th::TREE_ANGLE_START + (static_cast<float>(i) * th::TREE_ANGLE_STEP)) * th::DEG_TO_RAD_FACTOR;
        float treeX = center.x + (th::TREE_RING_RADIUS * cosf(angle));
        float treeZ = center.z + (th::TREE_RING_RADIUS * sinf(angle));

        // Trunk: small brown cylinder
        Vector3 trunkBase = {.x = treeX, .y = center.y, .z = treeZ};
        Vector3 trunkTop = {.x = treeX, .y = center.y + th::TREE_TRUNK_HEIGHT, .z = treeZ};
        DrawCylinderEx(trunkBase, trunkTop, th::TREE_TRUNK_RADIUS, th::TREE_TRUNK_RADIUS, th::CYLINDER_SLICES,
                       th::TREE_TRUNK_COLOR);

        // Canopy: green cone on top of trunk
        Vector3 canopyBase = trunkTop;
        Vector3 canopyTip = {.x = treeX, .y = center.y + th::TREE_TRUNK_HEIGHT + th::TREE_CANOPY_HEIGHT, .z = treeZ};
        DrawCylinderEx(canopyBase, canopyTip, th::TREE_CANOPY_RADIUS, 0.0F, th::CONE_SLICES, th::TREE_CANOPY_COLOR);
    }
}

/// Draw a tall gray-white peak on a mountain hex tile.
static void drawMountainPeak(Vector3 center) {
    namespace th = terrain_height;
    // Main rocky peak cone
    Vector3 peakBase = {.x = center.x, .y = center.y, .z = center.z};
    Vector3 peakTop = {.x = center.x, .y = center.y + th::MOUNTAIN_PEAK_HEIGHT, .z = center.z};
    DrawCylinderEx(peakBase, peakTop, th::MOUNTAIN_PEAK_RADIUS, 0.0F, th::CONE_SLICES, th::MOUNTAIN_PEAK_COLOR);

    // Snow cap: smaller white cone on top
    Vector3 snowBase = {.x = center.x, .y = center.y + th::MOUNTAIN_PEAK_HEIGHT - th::SNOW_CAP_HEIGHT, .z = center.z};
    DrawCylinderEx(snowBase, peakTop, th::SNOW_CAP_RADIUS, 0.0F, th::CONE_SLICES, th::MOUNTAIN_SNOW_COLOR);
}

/// Draw a rounded bump on a hills hex tile.
static void drawHillsBump(Vector3 center) {
    namespace th = terrain_height;
    DrawSphere(center, th::HILLS_BUMP_RADIUS, terrain_colors::HILLS_FILL);
}

/// Draw 3D terrain decorations on a visible tile.
static void drawDecorations(Vector3 center, game::TerrainType terrain) {
    switch (terrain) {
    case game::TerrainType::Forest:
        drawForestTrees(center);
        break;
    case game::TerrainType::Mountain:
        drawMountainPeak(center);
        break;
    case game::TerrainType::Hills:
        drawHillsBump(center);
        break;
    default:
        break;
    }
}

/// Draw outlines, highlights, and decorations for a single visible/explored tile.
static void drawTileOverlays(Vector3 center, game::TerrainType terrain, game::VisibilityState vis,
                             std::optional<hex::TileCoord> highlightedTile, int row, int col) {
    Color outlineColor = terrain_colors::terrainOutlineColor(terrain);
    if (vis == game::VisibilityState::Explored) {
        outlineColor = dimColor(outlineColor);
    }

    // Highlight hovered tile on top of terrain fill (only on visible tiles).
    if (vis == game::VisibilityState::Visible && highlightedTile && highlightedTile->row == row &&
        highlightedTile->col == col) {
        drawFilledHex3D(center, YELLOW);
    }

    drawHex3D(center, outlineColor);

    if (vis == game::VisibilityState::Visible) {
        drawDecorations(center, terrain);
    }
}

void drawMap(const game::Map &map, std::optional<hex::TileCoord> highlightedTile, const game::FogOfWar *fog,
             game::FactionId playerFactionId) {
    HexMeshBuilder builder;

    float innerRadius = hex_metrics::SOLID_FACTOR * hex_metrics::HEX_RADIUS;

    // ── Pass 1: Build blended terrain mesh ──────────────────────────────────
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            auto vis = game::VisibilityState::Visible;
            if (fog != nullptr) {
                vis = fog->getVisibility(playerFactionId, row, col);
            }
            if (vis == game::VisibilityState::Unexplored) {
                continue;
            }

            Vector3 center = hex::tileCenter(row, col);
            game::TerrainType terrain = map.tile(row, col).terrainType();
            center.y += terrain_height::terrainHeight(terrain);

            Color cellColor = terrain_colors::terrainFillColor(terrain);
            if (vis == game::VisibilityState::Explored) {
                cellColor = dimColor(cellColor);
            }

            auto innerCorners = hexCornersAtRadius(center, innerRadius);
            auto outerCorners = hexCornersAtRadius(center, hex_metrics::HEX_RADIUS);

            addInnerHex(builder, center, innerCorners, cellColor);
            addNearEdges(builder, map, row, col, innerCorners, outerCorners, cellColor, fog, playerFactionId, vis);
            addFarEdges(builder, map, row, col, innerCorners, outerCorners, cellColor, fog, playerFactionId);
            addNearCorners(builder, map, row, col, innerCorners, outerCorners, cellColor, fog, playerFactionId, vis);
            addFarCorners(builder, map, row, col, innerCorners, outerCorners, cellColor, fog, playerFactionId, vis);
        }
    }

    builder.flush();

    // ── Pass 2: Overlays, outlines, highlights, and decorations ─────────────
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            Vector3 center = hex::tileCenter(row, col);

            auto vis = game::VisibilityState::Visible;
            if (fog != nullptr) {
                vis = fog->getVisibility(playerFactionId, row, col);
            }

            if (vis == game::VisibilityState::Unexplored) {
                drawFilledHex3D(center, FOG_UNEXPLORED_COLOR);
                drawHex3D(center, FOG_UNEXPLORED_OUTLINE);
                continue;
            }

            game::TerrainType terrain = map.tile(row, col).terrainType();
            center.y += terrain_height::terrainHeight(terrain);

            drawTileOverlays(center, terrain, vis, highlightedTile, row, col);
        }
    }
}

} // namespace engine
