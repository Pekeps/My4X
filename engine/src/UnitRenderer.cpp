#include "engine/UnitRenderer.h"

#include "engine/HexGrid.h"

#include "raylib.h"

namespace engine {

// Unit marker dimensions.
const float UNIT_RADIUS = 0.3F;
const float UNIT_HEIGHT = 0.8F;
const float UNIT_Y_OFFSET = UNIT_HEIGHT * 0.5F; // Raise so the base sits on the ground.
const int UNIT_SLICES = 8;

void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units) {
    for (const auto &unit : units) {
        if (!unit->isAlive()) {
            continue;
        }

        Vector3 center = hex::tileCenter(unit->row(), unit->col());
        center.y = UNIT_Y_OFFSET;

        // Draw unit as a small cylinder standing on the hex tile.
        DrawCylinder(center, UNIT_RADIUS, UNIT_RADIUS, UNIT_HEIGHT, UNIT_SLICES, MAROON);
        DrawCylinderWires(center, UNIT_RADIUS, UNIT_RADIUS, UNIT_HEIGHT, UNIT_SLICES, RED);
    }
}

} // namespace engine
