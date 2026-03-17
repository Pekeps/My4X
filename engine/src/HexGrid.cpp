#include "engine/HexGrid.h"

#include <cmath>
#include <numbers>

namespace engine::hex {

const float HEX_RADIUS = 1.0F;
const float START_ANGLE_DEGREES = 30.0F;
const float DEGREES_PER_SIDE = 60.0F;
const float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0F;
const float HEX_WIDTH = HEX_RADIUS * 2.0F;
const float HEX_HEIGHT = std::numbers::sqrt3_v<float> * HEX_RADIUS;
const float HEX_THREE_QUARTER = 0.75F;
const float HEX_HORIZONTAL_SPACING = HEX_HEIGHT;
const float HEX_VERTICAL_SPACING = HEX_WIDTH * HEX_THREE_QUARTER;
const float ODD_ROW_OFFSET = HEX_HORIZONTAL_SPACING * 0.5F;

Vector3 tileCenter(int row, int col) {
    float centerX = static_cast<float>(col) * HEX_HORIZONTAL_SPACING;
    float centerZ = static_cast<float>(row) * HEX_VERTICAL_SPACING;

    // Odd-r offset: shift odd rows by half the horizontal spacing.
    if (row % 2 != 0) {
        centerX += ODD_ROW_OFFSET;
    }

    return {.x = centerX, .y = 0.0F, .z = centerZ};
}

std::array<Vector3, HEX_VERTEX_COUNT> hexVertices(Vector3 center) {
    std::array<Vector3, HEX_VERTEX_COUNT> verts{};
    for (int i = 0; i < HEX_VERTEX_COUNT; ++i) {
        float angle = (START_ANGLE_DEGREES + (static_cast<float>(i) * DEGREES_PER_SIDE)) * DEG_TO_RAD;
        verts[i] = {
            .x = center.x + (HEX_RADIUS * cosf(angle)),
            .y = 0.0F,
            .z = center.z + (HEX_RADIUS * sinf(angle)),
        };
    }
    return verts;
}

} // namespace engine::hex
