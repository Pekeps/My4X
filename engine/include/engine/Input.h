#pragma once

#include "engine/HexGrid.h"

#include "raylib.h"

#include <optional>

namespace engine::input {

// Casts a ray from the mouse position through the camera into the XZ plane (Y=0).
// Returns the world-space hit point, or nullopt if the ray doesn't hit the plane.
std::optional<Vector3> mouseToGround(Camera3D cam);

// Returns the tile coordinate under the mouse cursor, or nullopt if none.
std::optional<hex::TileCoord> mouseToTile(Camera3D cam, int mapHeight, int mapWidth);

} // namespace engine::input
