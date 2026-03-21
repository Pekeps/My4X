#include "engine/HexGrid.h"

#include <cmath>
#include <limits>
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
    std::array<Vector3, HEX_VERTEX_COUNT> verticies{};
    for (int i = 0; i < HEX_VERTEX_COUNT; ++i) {
        float angle = (START_ANGLE_DEGREES + (static_cast<float>(i) * DEGREES_PER_SIDE)) * DEG_TO_RAD;
        verticies.at(i) = {
            .x = center.x + (HEX_RADIUS * cosf(angle)),
            .y = center.y,
            .z = center.z + (HEX_RADIUS * sinf(angle)),
        };
    }
    return verticies;
}

std::optional<TileCoord> worldToTile(float worldX, float worldZ, int mapHeight, int mapWidth) {
    // Approximate row/col from world position, then check neighbors
    // to find the closest hex center (brute-force but reliable for odd-r offset grids).
    int approxRow = static_cast<int>(std::round(worldZ / HEX_VERTICAL_SPACING));
    int approxCol = static_cast<int>(std::round(worldX / HEX_HORIZONTAL_SPACING));

    int bestRow = -1;
    int bestCol = -1;
    float bestDist = std::numeric_limits<float>::max();

    // Check a small neighborhood around the approximation.
    for (int row = approxRow - 1; row <= approxRow + 1; ++row) {
        for (int col = approxCol - 1; col <= approxCol + 1; ++col) {
            if (row < 0 || row >= mapHeight || col < 0 || col >= mapWidth) {
                continue;
            }
            Vector3 center = tileCenter(row, col);
            float diffX = worldX - center.x;
            float diffZ = worldZ - center.z;
            float dist = (diffX * diffX) + (diffZ * diffZ);
            if (dist < bestDist) {
                bestDist = dist;
                bestRow = row;
                bestCol = col;
            }
        }
    }

    // Only accept if the point is within one hex radius of the closest center.
    if (bestRow >= 0 && bestDist <= (HEX_RADIUS * HEX_RADIUS)) {
        return TileCoord{.row = bestRow, .col = bestCol};
    }

    return std::nullopt;
}

} // namespace engine::hex
