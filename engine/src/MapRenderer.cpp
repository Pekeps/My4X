#include "engine/MapRenderer.h"

#include "raylib.h"

#include <cmath>
#include <numbers>

namespace engine {

// Hex grid layout constants.
// Using flat-top hexagons with offset coordinates (odd-r: odd rows shifted right).
// The hex grid lies on the XZ plane (Y = 0).
const float HEX_RADIUS = 1.0F;
const float HEX_WIDTH = HEX_RADIUS * 2.0F;
const float HEX_HEIGHT = std::numbers::sqrt3_v<float> * HEX_RADIUS;
const float HEX_THREE_QUARTER = 0.75F;
const float HEX_HORIZONTAL_SPACING = HEX_HEIGHT;
const float HEX_VERTICAL_SPACING = HEX_WIDTH * HEX_THREE_QUARTER;
const float ODD_ROW_OFFSET = HEX_HORIZONTAL_SPACING * 0.5F;

const int HEX_SIDES = 6;
const float START_ANGLE_DEGREES = 30.0F;
const float DEGREES_PER_SIDE = 60.0F;

// Compute the 3D center of a hex tile on the XZ plane.
Vector3 hexCenter3D(int row, int col) {
    float centerX = static_cast<float>(col) * HEX_HORIZONTAL_SPACING;
    float centerZ = static_cast<float>(row) * HEX_VERTICAL_SPACING;

    // Odd-r offset: shift odd rows by half the horizontal spacing.
    if (row % 2 != 0) {
        centerX += ODD_ROW_OFFSET;
    }

    return {centerX, 0.0F, centerZ};
}

void drawHex3D(Vector3 center, Color outline) {
    for (int i = 0; i < HEX_SIDES; ++i) {
        float angle1 = (START_ANGLE_DEGREES + (static_cast<float>(i) * DEGREES_PER_SIDE)) * DEG2RAD;
        float angle2 = (START_ANGLE_DEGREES + (static_cast<float>(i + 1) * DEGREES_PER_SIDE)) * DEG2RAD;

        Vector3 point1 = {
            center.x + (HEX_RADIUS * cosf(angle1)),
            0.0F,
            center.z + (HEX_RADIUS * sinf(angle1)),
        };
        Vector3 point2 = {
            center.x + (HEX_RADIUS * cosf(angle2)),
            0.0F,
            center.z + (HEX_RADIUS * sinf(angle2)),
        };

        DrawLine3D(point1, point2, outline);
    }
}

void drawMap(const game::Map &map) {
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            Vector3 center = hexCenter3D(row, col);
            drawHex3D(center, BROWN);
        }
    }
}

} // namespace engine
