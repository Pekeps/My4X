#pragma once

#include "raylib.h"

#include <array>

namespace engine::hex {

const int HEX_VERTEX_COUNT = 6;

// Hex grid layout constants for flat-top hexagons with odd-r offset coordinates.
// The grid lies on the XZ plane (Y = 0).
Vector3 tileCenter(int row, int col);

// Returns the 6 vertices of a hex at the given center, on the XZ plane.
std::array<Vector3, HEX_VERTEX_COUNT> hexVertices(Vector3 center);

} // namespace engine::hex
