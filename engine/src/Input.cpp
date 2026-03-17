#include "engine/Input.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>

namespace engine::input {

std::optional<Vector3> mouseToGround(Camera3D cam) {
    Ray ray = GetMouseRay(GetMousePosition(), cam);

    // Intersect ray with the XZ plane (Y = 0).
    // ray.position.y + t * ray.direction.y = 0
    if (std::abs(ray.direction.y) < 0.0001F) {
        return std::nullopt; // Ray is parallel to the ground.
    }

    float dist = -ray.position.y / ray.direction.y;
    if (dist < 0.0F) {
        return std::nullopt; // Hit point is behind the camera.
    }

    Vector3 hit = Vector3Add(ray.position, Vector3Scale(ray.direction, dist));
    return hit;
}

std::optional<hex::TileCoord> mouseToTile(Camera3D cam, int mapHeight, int mapWidth) {
    auto ground = mouseToGround(cam);
    if (!ground) {
        return std::nullopt;
    }
    return hex::worldToTile(ground->x, ground->z, mapHeight, mapWidth);
}

} // namespace engine::input
