#include <utility>

#include "engine/UnitRenderer.h"

#include "engine/DiplomacyColors.h"
#include "engine/FactionColors.h"
#include "engine/HexGrid.h"
#include "engine/ModelManager.h"

#include "raylib.h"
#include "raymath.h"

#include <cstddef>

namespace engine {

/// Y-offset to place the model at ground level.
static constexpr float MODEL_Y_OFFSET = 0.0F;

/// Uniform scale applied to unit models so they fit within a hex tile.
static constexpr float MODEL_SCALE = 0.6F;

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

// Fallback cylinder dimensions (used when no model is available).
static constexpr float FALLBACK_CYLINDER_RADIUS = 0.3F;
static constexpr float FALLBACK_CYLINDER_HEIGHT = 0.8F;
static constexpr float FALLBACK_Y_SCALE = 0.2F;
static const float FALLBACK_Y_OFFSET = FALLBACK_CYLINDER_HEIGHT * FALLBACK_Y_SCALE;
static constexpr int FALLBACK_CYLINDER_SLICES = 8;

/// Draw the unit's 3D model (or fallback cylinder) at its tile position.
static void drawUnitModel(const game::Unit &unit, const ModelManager &models, Color fillColor, Color wireColor) {
    Vector3 center = hex::tileCenter(unit.row(), unit.col());

    const auto &modelKey = unit.unitTemplate().modelKey;
    const Model *mdl = modelKey.empty() ? nullptr : models.getModel(modelKey);

    if (mdl != nullptr) {
        Vector3 modelPos = center;
        modelPos.y = MODEL_Y_OFFSET;
        DrawModel(*mdl, modelPos, MODEL_SCALE, fillColor);
        DrawModelWires(*mdl, modelPos, MODEL_SCALE, wireColor);
    } else {
        center.y = FALLBACK_Y_OFFSET;
        DrawCylinder(center, FALLBACK_CYLINDER_RADIUS, FALLBACK_CYLINDER_RADIUS, FALLBACK_CYLINDER_HEIGHT,
                     FALLBACK_CYLINDER_SLICES, fillColor);
        DrawCylinderWires(center, FALLBACK_CYLINDER_RADIUS, FALLBACK_CYLINDER_RADIUS, FALLBACK_CYLINDER_HEIGHT,
                          FALLBACK_CYLINDER_SLICES, wireColor);
    }
}

/// Draw level pips and level-up indicator for a unit.
static void drawLevelIndicators(const game::Unit &unit) {
    int unitLevel = unit.level();
    if (unitLevel > 0) {
        Vector3 basePos = hex::tileCenter(unit.row(), unit.col());
        float totalWidth = static_cast<float>(unitLevel - 1) * LEVEL_PIP_SPREAD;
        float startX = basePos.x - (totalWidth / PIP_CENTER_DIVISOR);
        for (int pip = 0; pip < unitLevel; ++pip) {
            Vector3 pipPos = {startX + (static_cast<float>(pip) * LEVEL_PIP_SPREAD), LEVEL_PIP_Y, basePos.z};
            DrawSphere(pipPos, LEVEL_PIP_RADIUS, GOLD);
        }
    }

    if (unit.justLeveledUp()) {
        Vector3 starPos = hex::tileCenter(unit.row(), unit.col());
        starPos.y = LEVEL_STAR_Y_OFFSET;
        DrawSphere(starPos, LEVEL_STAR_RADIUS, GOLD);
    }
}

/// Resolve the faction fill and wire colors for a unit.
static void resolveFactionColors(const game::Unit &unit, const game::FactionRegistry &factions, Color &fillColor,
                                 Color &wireColor) {
    fillColor = MAROON;
    wireColor = RED;
    const auto *faction = factions.findFaction(unit.factionId());
    if (faction != nullptr) {
        fillColor = faction_colors::factionColor(faction->type(), faction->colorIndex());
        wireColor = faction_colors::factionWireColor(faction->type(), faction->colorIndex());
    }
}

void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units, const game::FactionRegistry &factions,
               const ModelManager &models, int selectedIndex, game::FactionId playerFactionId,
               const game::DiplomacyManager *diplomacy, const game::FogOfWar *fog) {
    for (std::size_t i = 0; i < units.size(); ++i) {
        const auto &unit = units.at(i);
        if (!unit->isAlive()) {
            continue;
        }

        // Hide enemy units on non-Visible tiles (fog of war).
        if (fog != nullptr && unit->factionId() != playerFactionId) {
            auto vis = fog->getVisibility(playerFactionId, unit->row(), unit->col());
            if (vis != game::VisibilityState::Visible) {
                continue;
            }
        }

        Color fillColor{};
        Color wireColor{};
        resolveFactionColors(*unit, factions, fillColor, wireColor);

        drawUnitModel(*unit, models, fillColor, wireColor);

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

        drawLevelIndicators(*unit);
    }
}

} // namespace engine
