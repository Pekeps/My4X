#include <utility>

#include "engine/UnitRenderer.h"

#include "engine/DiplomacyColors.h"
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

// Level-up indicator: golden sphere above the unit.
static constexpr float LEVEL_STAR_RADIUS = 0.12F;
static constexpr float LEVEL_STAR_Y_OFFSET = 1.15F;

// Level pips: small spheres at the base of the unit indicating level.
static constexpr float LEVEL_PIP_RADIUS = 0.06F;
static constexpr float LEVEL_PIP_Y = 0.05F;
static constexpr float LEVEL_PIP_SPREAD = 0.15F;
static constexpr float PIP_CENTER_DIVISOR = 2.0F;

void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units, const game::FactionRegistry &factions,
               int selectedIndex, game::FactionId playerFactionId, const game::DiplomacyManager *diplomacy) {
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

        // Draw diplomacy indicator ring for foreign units.
        if (diplomacy != nullptr && unit->factionId() != playerFactionId) {
            auto relation = diplomacy->getRelation(playerFactionId, unit->factionId());
            Color dipColor = diplomacy_colors::diplomacyColor(relation);
            Vector3 ringPos = hex::tileCenter(unit->row(), unit->col());
            DrawCylinder(ringPos, diplomacy_colors::DIPLOMACY_RING_RADIUS, diplomacy_colors::DIPLOMACY_RING_RADIUS,
                         diplomacy_colors::DIPLOMACY_RING_HEIGHT, diplomacy_colors::DIPLOMACY_RING_SLICES, dipColor);
        }

        // Draw level pips (small golden spheres) for leveled units.
        int unitLevel = unit->level();
        if (unitLevel > 0) {
            Vector3 basePos = hex::tileCenter(unit->row(), unit->col());
            // Center the pips around the unit base.
            float totalWidth = static_cast<float>(unitLevel - 1) * LEVEL_PIP_SPREAD;
            float startX = basePos.x - (totalWidth / PIP_CENTER_DIVISOR);
            for (int pip = 0; pip < unitLevel; ++pip) {
                Vector3 pipPos = {startX + (static_cast<float>(pip) * LEVEL_PIP_SPREAD), LEVEL_PIP_Y, basePos.z};
                DrawSphere(pipPos, LEVEL_PIP_RADIUS, GOLD);
            }
        }

        // Draw level-up indicator (glowing sphere above unit) when just leveled up.
        if (unit->justLeveledUp()) {
            Vector3 starPos = hex::tileCenter(unit->row(), unit->col());
            starPos.y = LEVEL_STAR_Y_OFFSET;
            DrawSphere(starPos, LEVEL_STAR_RADIUS, GOLD);
        }
    }
}

} // namespace engine
