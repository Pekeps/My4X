#include "engine/MapRenderer.h"

#include "HexDraw.h"
#include "engine/HexGrid.h"

#include "raylib.h"

namespace engine {

void drawMap(const game::Map &map, std::optional<hex::TileCoord> highlightedTile) {
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            Vector3 center = hex::tileCenter(row, col);

            if (highlightedTile && highlightedTile->row == row && highlightedTile->col == col) {
                drawFilledHex3D(center, YELLOW);
            }

            drawHex3D(center, BROWN);
        }
    }
}

} // namespace engine
