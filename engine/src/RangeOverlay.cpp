#include "engine/RangeOverlay.h"

#include "HexDraw.h"
#include "engine/HexGrid.h"

#include "game/AttackAction.h"
#include "game/Map.h"
#include "game/Pathfinding.h"
#include "game/Unit.h"

#include "raylib.h"

namespace engine {

namespace {

/// Minimum attack range for a unit to be considered ranged.
constexpr int RANGED_THRESHOLD = 2;

/// Color for the attack range overlay (semi-transparent orange-red).
const Color ATTACK_RANGE_COLOR = {255, 120, 40, 60};

/// Color for the movement range overlay (semi-transparent blue).
const Color MOVEMENT_RANGE_COLOR = {40, 120, 255, 60};

} // namespace

void drawAttackRangeOverlay(const game::Unit &unit, const game::Map &map) {
    int range = unit.attackRange();

    // Only draw overlay for ranged units (range >= 2).
    if (range < RANGED_THRESHOLD) {
        return;
    }

    int unitRow = unit.row();
    int unitCol = unit.col();

    // Iterate over all tiles within attack range and highlight them.
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            // Skip the unit's own tile.
            if (row == unitRow && col == unitCol) {
                continue;
            }

            int dist = game::AttackAction::hexDistance(unitRow, unitCol, row, col);
            if (dist <= range) {
                Vector3 center = hex::tileCenter(row, col);
                drawFilledHex3D(center, ATTACK_RANGE_COLOR);
            }
        }
    }
}

void drawMovementRangeOverlay(const std::vector<game::ReachableTile> &reachableTiles) {
    for (const auto &tile : reachableTiles) {
        Vector3 center = hex::tileCenter(tile.coord.row, tile.coord.col);
        drawFilledHex3D(center, MOVEMENT_RANGE_COLOR);
    }
}

} // namespace engine
