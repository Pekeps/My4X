#pragma once

#include "raylib.h"

namespace engine {

/// Five vertices defining a subdivided hex edge (4 segments).
/// v1 is the "left" corner, v5 is the "right" corner.
/// v3 is the CENTER midpoint (used for river channels).
struct EdgeVertices {
    Vector3 v1;
    Vector3 v2;
    Vector3 v3;
    Vector3 v4;
    Vector3 v5;
};

/// Linearly interpolate between two Vector3 values.
[[nodiscard]] inline Vector3 lerpVector3(Vector3 a, Vector3 b, float t) {
    return {
        .x = a.x + ((b.x - a.x) * t),
        .y = a.y + ((b.y - a.y) * t),
        .z = a.z + ((b.z - a.z) * t),
    };
}

/// Linearly interpolate between two Colors.
[[nodiscard]] inline Color lerpColor(Color a, Color b, float t) {
    return {
        .r = static_cast<unsigned char>(static_cast<float>(a.r) +
                                        ((static_cast<float>(b.r) - static_cast<float>(a.r)) * t)),
        .g = static_cast<unsigned char>(static_cast<float>(a.g) +
                                        ((static_cast<float>(b.g) - static_cast<float>(a.g)) * t)),
        .b = static_cast<unsigned char>(static_cast<float>(a.b) +
                                        ((static_cast<float>(b.b) - static_cast<float>(a.b)) * t)),
        .a = static_cast<unsigned char>(static_cast<float>(a.a) +
                                        ((static_cast<float>(b.a) - static_cast<float>(a.a)) * t)),
    };
}

/// Create EdgeVertices by subdividing corner1->corner2 into 4 equal segments.
[[nodiscard]] inline EdgeVertices makeEdge(Vector3 corner1, Vector3 corner2) {
    static constexpr float QUARTER = 0.25F;
    static constexpr float HALF = 0.5F;
    static constexpr float THREE_QUARTER = 0.75F;
    return {
        .v1 = corner1,
        .v2 = lerpVector3(corner1, corner2, QUARTER),
        .v3 = lerpVector3(corner1, corner2, HALF),
        .v4 = lerpVector3(corner1, corner2, THREE_QUARTER),
        .v5 = corner2,
    };
}

} // namespace engine
