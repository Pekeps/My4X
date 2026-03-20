#include "game/MoveAction.h"

#include "game/AttackAction.h"
#include "game/GameState.h"
#include "game/TerrainType.h"
#include "game/Unit.h"

namespace game {

/// Adjacent tiles must be at hex distance 1.
static constexpr int ADJACENT_DISTANCE = 1;

MoveAction::MoveAction(std::size_t unitIndex, int destRow, int destCol)
    : unitIndex_(unitIndex), destRow_(destRow), destCol_(destCol) {}

MoveValidation MoveAction::validate(const GameState &state) const {
    const auto &units = state.units();

    if (unitIndex_ >= units.size()) {
        return MoveValidation::UnitNotFound;
    }

    const auto &unit = *units[unitIndex_];

    if (!unit.isAlive()) {
        return MoveValidation::UnitDead;
    }

    if (unit.movementRemaining() <= 0) {
        return MoveValidation::NoMovementRemaining;
    }

    // Must be adjacent (hex distance 1).
    int dist = AttackAction::hexDistance(unit.row(), unit.col(), destRow_, destCol_);
    if (dist != ADJACENT_DISTANCE) {
        return MoveValidation::NotAdjacent;
    }

    // Check terrain passability and movement cost.
    const auto &destTile = state.map().tile(destRow_, destCol_);
    const auto &terrainProps = getTerrainProperties(destTile.terrainType());

    if (!terrainProps.passable) {
        return MoveValidation::ImpassableTerrain;
    }

    if (unit.movementRemaining() < terrainProps.movementCost) {
        return MoveValidation::InsufficientMovement;
    }

    return MoveValidation::Valid;
}

MoveResult MoveAction::execute(GameState &state) const {
    MoveResult result;
    result.validation = validate(state);

    if (result.validation != MoveValidation::Valid) {
        result.executed = false;
        return result;
    }

    auto &units = state.units();
    auto &unit = *units[unitIndex_];

    const auto &destTile = state.map().tile(destRow_, destCol_);
    const auto &terrainProps = getTerrainProperties(destTile.terrainType());

    // Unregister from old tile, move, register on new tile.
    state.mutableRegistry().unregisterUnit(unit.row(), unit.col(), &unit);
    unit.moveTo(destRow_, destCol_, terrainProps.movementCost);
    state.mutableRegistry().registerUnit(unit.row(), unit.col(), &unit);

    result.executed = true;
    result.movementCostDeducted = terrainProps.movementCost;
    return result;
}

} // namespace game
