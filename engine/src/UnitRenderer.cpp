#include <utility>

#include "engine/UnitRenderer.h"

#include "engine/HexGrid.h"

#include "raylib.h"

namespace engine {

const float UNIT_RADIUS = 0.3F;
const float UNIT_HEIGHT = 0.8F;
const float UNIT_Y_OFFSET = UNIT_HEIGHT * 0.2F;
const int UNIT_SLICES = 8;

// Selection ring drawn around the selected unit.
const float RING_RADIUS = 0.5F;
const float RING_HEIGHT = 0.05F;
const int RING_SLICES = 16;

void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units, int selectedIndex) {
    for (int i = 0; i < units.size(); ++i) {
        const auto &unit = units.at(i);
        if (!unit->isAlive()) {
            continue;
        }

        Vector3 center = hex::tileCenter(unit->row(), unit->col());
        center.y = UNIT_Y_OFFSET;

        DrawCylinder(center, UNIT_RADIUS, UNIT_RADIUS, UNIT_HEIGHT, UNIT_SLICES, MAROON);
        DrawCylinderWires(center, UNIT_RADIUS, UNIT_RADIUS, UNIT_HEIGHT, UNIT_SLICES, RED);

        // Draw selection ring on the ground under the selected unit.
        if (i == selectedIndex) {
            Vector3 ringPos = hex::tileCenter(unit->row(), unit->col());
            DrawCylinder(ringPos, RING_RADIUS, RING_RADIUS, RING_HEIGHT, RING_SLICES, YELLOW);
        }
    }
}

} // namespace engine
