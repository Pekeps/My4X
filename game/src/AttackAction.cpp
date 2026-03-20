#include "game/AttackAction.h"

#include "game/Combat.h"
#include "game/DiplomacyManager.h"
#include "game/GameState.h"
#include "game/Map.h"
#include "game/TerrainType.h"
#include "game/Unit.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace game {

namespace {

/// Divisor used when converting odd-r offset coordinates to cube coordinates.
/// Odd rows are shifted by half a column in the hex grid layout.
constexpr int OFFSET_DIVISOR = 2;

} // namespace

AttackAction::AttackAction(std::size_t attackerIndex, std::size_t targetIndex)
    : attackerIndex_(attackerIndex), targetIndex_(targetIndex) {}

AttackValidation AttackAction::validate(const GameState &state) const {
    const auto &units = state.units();

    // Check indices are valid.
    if (attackerIndex_ >= units.size()) {
        return AttackValidation::AttackerNotFound;
    }
    if (targetIndex_ >= units.size()) {
        return AttackValidation::TargetNotFound;
    }

    const auto &attacker = *units[attackerIndex_];
    const auto &target = *units[targetIndex_];

    // Check alive status.
    if (!attacker.isAlive()) {
        return AttackValidation::AttackerDead;
    }
    if (!target.isAlive()) {
        return AttackValidation::TargetDead;
    }

    // Cannot attack own faction.
    if (attacker.factionId() == target.factionId()) {
        return AttackValidation::SameFaction;
    }

    // Must be at war with target's faction.
    if (!state.diplomacy().areAtWar(attacker.factionId(), target.factionId())) {
        return AttackValidation::NotAtWar;
    }

    // Check movement/action points.
    if (attacker.movementRemaining() < ATTACK_MOVEMENT_COST) {
        return AttackValidation::NoMovementRemaining;
    }

    // Check range.
    int distance = hexDistance(attacker.row(), attacker.col(), target.row(), target.col());
    int maxRange = attacker.attackRange();

    // Melee units must be adjacent; ranged units use their attack range.
    if (distance > maxRange) {
        return AttackValidation::OutOfRange;
    }

    return AttackValidation::Valid;
}

AttackResult AttackAction::execute(GameState &state) const {
    AttackResult result;
    result.validation = validate(state);

    if (result.validation != AttackValidation::Valid) {
        result.executed = false;
        return result;
    }

    auto &units = state.units();
    auto &attacker = *units[attackerIndex_];
    auto &target = *units[targetIndex_];

    // Determine if this is a ranged attack (distance > melee range).
    int distance = hexDistance(attacker.row(), attacker.col(), target.row(), target.col());
    CombatContext context;
    context.rangedAttack = (distance > MELEE_RANGE);

    // Look up terrain defense bonuses from the map.
    const auto &map = state.map();
    const auto &defenderTerrain = map.tile(target.row(), target.col()).terrainType();
    context.terrainDefenseBonus = getTerrainProperties(defenderTerrain).defenseModifier;

    const auto &attackerTerrain = map.tile(attacker.row(), attacker.col()).terrainType();
    context.attackerTerrainDefenseBonus = getTerrainProperties(attackerTerrain).defenseModifier;

    // Resolve combat (pure function — does not mutate units).
    result.combat = resolveCombat(attacker, target, context);
    result.executed = true;

    // Apply damage.
    target.takeDamage(result.combat.damageToDefender);
    attacker.takeDamage(result.combat.damageToAttacker);

    // Award experience points based on combat outcome.
    if (attacker.isAlive()) {
        int attackerXp = result.combat.damageToDefender * XP_PER_DAMAGE_DEALT;
        if (result.combat.defenderDied) {
            attackerXp += XP_KILL_BONUS;
        }
        attacker.addExperience(attackerXp);
    }
    if (target.isAlive()) {
        int defenderXp = XP_DEFEND_SURVIVE;
        if (result.combat.damageToAttacker > 0) {
            defenderXp += result.combat.damageToAttacker * XP_PER_DAMAGE_DEALT;
        }
        if (result.combat.attackerDied) {
            defenderXp += XP_KILL_BONUS;
        }
        target.addExperience(defenderXp);
    }

    // Deduct movement points from attacker (if still alive).
    int newMovement = attacker.movementRemaining() - ATTACK_MOVEMENT_COST;
    attacker.setMovementRemaining(std::max(0, newMovement));

    // Remove dead units from the game. Remove higher index first to avoid
    // invalidating the lower index.
    std::size_t firstToRemove = std::max(attackerIndex_, targetIndex_);
    std::size_t secondToRemove = std::min(attackerIndex_, targetIndex_);

    if (firstToRemove != secondToRemove) {
        // Two different units — check both.
        const auto &firstUnit = *units[firstToRemove];
        const auto &secondUnit = *units[secondToRemove];

        if (!firstUnit.isAlive()) {
            state.removeUnit(firstToRemove);
        }
        if (!secondUnit.isAlive()) {
            // If we already removed the higher-indexed unit, the lower index
            // is still valid.
            state.removeUnit(secondToRemove);
        }
    }

    return result;
}

int AttackAction::hexDistance(int row1, int col1, int row2, int col2) {
    // Convert odd-r offset coordinates to cube coordinates.
    // For odd-r: x = col - (row - (row & 1)) / 2
    //            z = row
    //            y = -x - z
    int x1 = col1 - ((row1 - (row1 & 1)) / OFFSET_DIVISOR);
    int z1 = row1;
    int y1 = -x1 - z1;

    int x2 = col2 - ((row2 - (row2 & 1)) / OFFSET_DIVISOR);
    int z2 = row2;
    int y2 = -x2 - z2;

    return (std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(z1 - z2)) / OFFSET_DIVISOR;
}

} // namespace game
