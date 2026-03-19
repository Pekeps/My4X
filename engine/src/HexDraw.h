#pragma once
#include "engine/HexGrid.h"
#include "raylib.h"
namespace engine {
inline void drawHex3D(Vector3 center, Color outline) {
    auto verts = hex::hexVertices(center);
    for (int curr = 0; curr < hex::HEX_VERTEX_COUNT; ++curr) {
        int next = (curr + 1) % hex::HEX_VERTEX_COUNT;
        DrawLine3D(verts.at(curr), verts.at(next), outline);
    }
}
inline void drawFilledHex3D(Vector3 center, Color fill) {
    auto verts = hex::hexVertices(center);
    for (int curr = 0; curr < hex::HEX_VERTEX_COUNT; ++curr) {
        int next = (curr + 1) % hex::HEX_VERTEX_COUNT;
        DrawTriangle3D(center, verts.at(next), verts.at(curr), fill);
    }
}
} // namespace engine
