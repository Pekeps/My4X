#include "game/Combat.h"

#include <algorithm>

namespace game {

namespace {

/// Compute damage dealt by an attacker to a defender, accounting for the
/// defender's effective defense (base defense + terrain bonus).
///
/// Formula: damage = max(MINIMUM_DAMAGE, attacker.attack - effectiveDefense / DEFENSE_DIVISOR)
[[nodiscard]] int computeDamage(int attackStat, int defenseStat, int terrainBonus) {
    const int effectiveDefense = defenseStat + terrainBonus;
    const int rawDamage = attackStat - (effectiveDefense / DEFENSE_DIVISOR);
    return std::max(MINIMUM_DAMAGE, rawDamage);
}

} // namespace

CombatResult resolveCombat(const Unit &attacker, const Unit &defender, const CombatContext &context) {
    CombatResult result;

    // ── Primary attack ──────────────────────────────────────────────────────
    result.damageToDefender = computeDamage(attacker.attack(), defender.defense(), context.terrainDefenseBonus);

    // Clamp to defender's remaining health so we don't report phantom damage.
    result.damageToDefender = std::min(result.damageToDefender, defender.health());
    result.defenderDied = (result.damageToDefender >= defender.health());

    // ── Counter-attack ──────────────────────────────────────────────────────
    // The defender strikes back only if:
    //   1. The defender is a melee unit (attackRange == 1).
    //   2. The defender survived the primary attack.
    static constexpr int MELEE_RANGE = 1;
    const bool defenderIsMelee = (defender.attackRange() == MELEE_RANGE);

    if (defenderIsMelee && !result.defenderDied) {
        result.damageToAttacker =
            computeDamage(defender.attack(), attacker.defense(), context.attackerTerrainDefenseBonus);

        // Clamp to attacker's remaining health.
        result.damageToAttacker = std::min(result.damageToAttacker, attacker.health());
        result.attackerDied = (result.damageToAttacker >= attacker.health());
    }

    return result;
}

} // namespace game
