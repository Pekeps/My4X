#pragma once

#include "raylib.h"

namespace engine {

/// Four vertices defining a subdivided hex edge (3 segments).
/// v1 is the "left" corner, v4 is the "right" corner.
/// v2 and v3 are the intermediate subdivision points.
struct EdgeVertices {
    Vector3 v1;
    Vector3 v2;
    Vector3 v3;
    Vector3 v4;
};

/// Linearly interpolate between two Vector3 values.
[[nodiscard]] inline Vector3 lerpVector3(Vector3 a, Vector3 b, float t) {
    return {
        .x = a.x + (b.x - a.x) * t,
        .y = a.y + (b.y - a.y) * t,
        .z = a.z + (b.z - a.z) * t,
    };
}

/// Linearly interpolate between two Colors.
[[nodiscard]] inline Color lerpColor(Color a, Color b, float t) {
    return {
        .r = static_cast<unsigned char>(static_cast<float>(a.r) +
                                        (static_cast<float>(b.r) - static_cast<float>(a.r)) * t),
        .g = static_cast<unsigned char>(static_cast<float>(a.g) +
                                        (static_cast<float>(b.g) - static_cast<float>(a.g)) * t),
        .b = static_cast<unsigned char>(static_cast<float>(a.b) +
                                        (static_cast<float>(b.b) - static_cast<float>(a.b)) * t),
        .a = static_cast<unsigned char>(static_cast<float>(a.a) +
                                        (static_cast<float>(b.a) - static_cast<float>(a.a)) * t),
    };
}

/// Create an EdgeVertices by subdividing the line from corner1 to corner2 into 3 equal segments.
[[nodiscard]] inline EdgeVertices makeEdge(Vector3 corner1, Vector3 corner2) {
    static constexpr float ONE_THIRD = 1.0F / 3.0F;
    static constexpr float TWO_THIRDS = 2.0F / 3.0F;
    return {
        .v1 = corner1,
        .v2 = lerpVector3(corner1, corner2, ONE_THIRD),
        .v3 = lerpVector3(corner1, corner2, TWO_THIRDS),
        .v4 = corner2,
    };
}

} // namespace engine
