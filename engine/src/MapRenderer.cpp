#include "engine/MapRenderer.h"

#include "engine/HexGrid.h"

#include "raylib.h"

namespace engine {

void drawHex3D(Vector3 center, Color outline) {
    auto verts = hex::hexVertices(center);
    for (int curr = 0; curr < hex::HEX_VERTEX_COUNT; ++curr) {
        int next = (curr + 1) % hex::HEX_VERTEX_COUNT;
        DrawLine3D(verts.at(curr), verts.at(next), outline);
    }
}

void drawFilledHex3D(Vector3 center, Color fill) {
    auto verts = hex::hexVertices(center);
    // Draw as a triangle fan from center. Draw both winding orders
    // so the fill is visible from above and below.
    for (int curr = 0; curr < hex::HEX_VERTEX_COUNT; ++curr) {
        int next = (curr + 1) % hex::HEX_VERTEX_COUNT;
        DrawTriangle3D(center, verts.at(next), verts.at(curr), fill);
    }
}

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
