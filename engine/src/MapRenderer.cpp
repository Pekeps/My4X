#include "engine/MapRenderer.h"

#include "HexDraw.h"
#include "engine/HexGrid.h"
#include "engine/TerrainColors.h"

#include "raylib.h"

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

static Color dimColor(Color base) {
    return Color{
        .r = static_cast<unsigned char>(static_cast<float>(base.r) * EXPLORED_DIM_FACTOR),
        .g = static_cast<unsigned char>(static_cast<float>(base.g) * EXPLORED_DIM_FACTOR),
        .b = static_cast<unsigned char>(static_cast<float>(base.b) * EXPLORED_DIM_FACTOR),
        .a = base.a,
    };
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
        }
    }
}

} // namespace engine
