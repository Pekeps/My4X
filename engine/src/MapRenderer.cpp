#include "engine/MapRenderer.h"

#include "HexDraw.h"
#include "engine/HexGrid.h"
#include "engine/TerrainColors.h"

#include "raylib.h"

namespace engine {

void drawMap(const game::Map &map, std::optional<hex::TileCoord> highlightedTile) {
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            Vector3 center = hex::tileCenter(row, col);
            const auto &tile = map.tile(row, col);
            game::TerrainType terrain = tile.terrainType();

            // Fill each hex with its terrain color.
            drawFilledHex3D(center, terrain_colors::terrainFillColor(terrain));

            // Highlight hovered tile on top of terrain fill.
            if (highlightedTile && highlightedTile->row == row && highlightedTile->col == col) {
                drawFilledHex3D(center, YELLOW);
            }

            // Draw outline using terrain-specific darker shade.
            drawHex3D(center, terrain_colors::terrainOutlineColor(terrain));
        }
    }
}

} // namespace engine
