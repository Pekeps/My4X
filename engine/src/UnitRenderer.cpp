#include <utility>

#include "engine/UnitRenderer.h"

#include "engine/FactionColors.h"
#include "engine/HexGrid.h"

#include "raylib.h"

#include <cstddef>

namespace engine {

static constexpr float UNIT_RADIUS = 0.3F;
static constexpr float UNIT_HEIGHT = 0.8F;
static constexpr float UNIT_Y_SCALE = 0.2F;
static const float UNIT_Y_OFFSET = UNIT_HEIGHT * UNIT_Y_SCALE;
static constexpr int UNIT_SLICES = 8;

// Selection ring drawn around the selected unit.
static constexpr float RING_RADIUS = 0.5F;
static constexpr float RING_HEIGHT = 0.05F;
static constexpr int RING_SLICES = 16;

void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units, const game::FactionRegistry &factions,
               int selectedIndex) {
    for (std::size_t i = 0; i < units.size(); ++i) {
        const auto &unit = units.at(i);
        if (!unit->isAlive()) {
            continue;
        }

        // Look up faction color for this unit.
        auto fillColor = MAROON;
        auto wireColor = RED;
        const auto *faction = factions.findFaction(unit->factionId());
        if (faction != nullptr) {
            fillColor = faction_colors::factionColor(faction->type(), faction->colorIndex());
            wireColor = faction_colors::factionWireColor(faction->type(), faction->colorIndex());
        }

        Vector3 center = hex::tileCenter(unit->row(), unit->col());
        center.y = UNIT_Y_OFFSET;

        DrawCylinder(center, UNIT_RADIUS, UNIT_RADIUS, UNIT_HEIGHT, UNIT_SLICES, fillColor);
        DrawCylinderWires(center, UNIT_RADIUS, UNIT_RADIUS, UNIT_HEIGHT, UNIT_SLICES, wireColor);

        // Draw selection ring on the ground under the selected unit.
        if (std::cmp_equal(i, selectedIndex)) {
            Vector3 ringPos = hex::tileCenter(unit->row(), unit->col());
            DrawCylinder(ringPos, RING_RADIUS, RING_RADIUS, RING_HEIGHT, RING_SLICES, YELLOW);
        }
    }
}

} // namespace engine
