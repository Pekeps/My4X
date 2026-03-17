#pragma once

#include "raylib.h"

#include <array>
#include <optional>

namespace engine::hex {

const int HEX_VERTEX_COUNT = 6;

struct TileCoord {
    int row;
    int col;
};

// Hex grid layout constants for flat-top hexagons with odd-r offset coordinates.
// The grid lies on the XZ plane (Y = 0).
Vector3 tileCenter(int row, int col);

// Returns the 6 vertices of a hex at the given center, on the XZ plane.
std::array<Vector3, HEX_VERTEX_COUNT> hexVertices(Vector3 center);

// Given a point on the XZ plane, find which hex tile it falls in.
// Returns nullopt if the point is outside the map bounds.
std::optional<TileCoord> worldToTile(float worldX, float worldZ, int mapHeight, int mapWidth);

} // namespace engine::hex
