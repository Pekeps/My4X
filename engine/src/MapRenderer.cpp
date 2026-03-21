#include "engine/MapRenderer.h"

#include "HexDraw.h"
#include "engine/HexGrid.h"
#include "engine/TerrainColors.h"
#include "engine/TerrainHeight.h"

#include "raylib.h"

#include <cmath>

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

static Color dimColor(Color base) {
    return Color{
        .r = static_cast<unsigned char>(static_cast<float>(base.r) * EXPLORED_DIM_FACTOR),
        .g = static_cast<unsigned char>(static_cast<float>(base.g) * EXPLORED_DIM_FACTOR),
        .b = static_cast<unsigned char>(static_cast<float>(base.b) * EXPLORED_DIM_FACTOR),
        .a = base.a,
    };
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

void drawMap(const game::Map &map, std::optional<hex::TileCoord> highlightedTile, const game::FogOfWar *fog,
             game::FactionId playerFactionId) {
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            Vector3 center = hex::tileCenter(row, col);
            const auto &tile = map.tile(row, col);
            game::TerrainType terrain = tile.terrainType();

            // Determine visibility state for this tile.
            auto vis = game::VisibilityState::Visible;
            if (fog != nullptr) {
                vis = fog->getVisibility(playerFactionId, row, col);
            }

            if (vis == game::VisibilityState::Unexplored) {
                // Unexplored: solid black hex — no terrain info revealed.
                drawFilledHex3D(center, FOG_UNEXPLORED_COLOR);
                drawHex3D(center, FOG_UNEXPLORED_OUTLINE);
                continue;
            }

            // Apply terrain height offset to the Y position.
            float heightOffset = terrain_height::terrainHeight(terrain);
            center.y += heightOffset;

            // Explored or Visible: draw terrain.
            Color fillColor = terrain_colors::terrainFillColor(terrain);
            Color outlineColor = terrain_colors::terrainOutlineColor(terrain);

            if (vis == game::VisibilityState::Explored) {
                // Dim terrain colors for explored-but-not-visible tiles.
                fillColor = dimColor(fillColor);
                outlineColor = dimColor(outlineColor);
            }

            drawFilledHex3D(center, fillColor);

            // Highlight hovered tile on top of terrain fill (only on visible tiles).
            if (vis == game::VisibilityState::Visible && highlightedTile && highlightedTile->row == row &&
                highlightedTile->col == col) {
                drawFilledHex3D(center, YELLOW);
            }

            drawHex3D(center, outlineColor);

            // Draw 3D decorations on visible tiles.
            if (vis == game::VisibilityState::Visible) {
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
        }
    }
}

} // namespace engine
