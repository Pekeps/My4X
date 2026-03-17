#include "engine/MapRenderer.h"

#include "engine/HexGrid.h"

#include "raylib.h"

namespace engine {

void drawHex3D(Vector3 center, Color outline) {
    auto verticies = hex::hexVertices(center);
    for (int curr = 0; curr < hex::HEX_VERTEX_COUNT; ++curr) {
        int next = (curr + 1) % hex::HEX_VERTEX_COUNT;

        DrawLine3D(verticies.at(curr), verticies.at(next), outline);
    }
}

void drawMap(const game::Map &map) {
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            Vector3 center = hex::tileCenter(row, col);
            drawHex3D(center, BROWN);
        }
    }
}

} // namespace engine
