#pragma once

#include "game/Unit.h"

namespace game {

/// Result of a single combat engagement between an attacker and a defender.
struct CombatResult {
    int damageToDefender = 0;
    int damageToAttacker = 0;
    bool defenderDied = false;
    bool attackerDied = false;
};

/// Contextual modifiers that influence combat resolution.
///
/// This struct provides hooks for terrain defense bonuses and other situational
/// modifiers without coupling the combat formula to the terrain or map systems.
/// Pass default-constructed CombatContext for unmodified combat.
struct CombatContext {
    /// Additive defense bonus applied to the defender from terrain (e.g. hills,
    /// forests). A value of 3 means +3 effective defense. The actual terrain
    /// mapping is handled by the caller — this is just the numeric bonus.
    int terrainDefenseBonus = 0;

    /// Additive defense bonus applied to the attacker from terrain when
    /// receiving a counter-attack.
    int attackerTerrainDefenseBonus = 0;

    /// If true, the attack is from outside melee range (ranged attack).
    /// Suppresses defender counter-attack even if the defender is melee.
    bool rangedAttack = false;
};

// ── Combat formula constants ────────────────────────────────────────────────
//
// Damage formula (deterministic):
//
//   effective_defense = defender.defense + terrain_bonus
//   raw_damage        = attacker.attack - effective_defense / 2
//   damage            = max(MINIMUM_DAMAGE, raw_damage)
//
// The defender's defense is halved before being subtracted from attack, so
// attack power matters more than defense. This keeps combat decisive while
// still rewarding defensive terrain and stats.
//
// Counter-attack: if the defender is melee (attackRange == 1) and survives the
// initial strike, it hits back using the same formula with roles reversed
// (defender attacks, original attacker defends). Ranged defenders do NOT
// counter-attack — they are vulnerable in melee.

/// Minimum damage any attack will deal, ensuring combat always progresses.
static constexpr int MINIMUM_DAMAGE = 1;

/// Divisor applied to the defender's effective defense in the damage formula.
/// A value of 2 means defense is halved before being subtracted from attack.
static constexpr int DEFENSE_DIVISOR = 2;

/// Resolve a single combat round between attacker and defender.
///
/// This is a pure function: it computes the result without mutating either unit.
/// The caller is responsible for applying damage (via Unit::takeDamage) and
/// removing dead units from the game state.
///
/// @param attacker The unit initiating the attack.
/// @param defender The unit being attacked.
/// @param context  Terrain and situational modifiers (default = no modifiers).
/// @return CombatResult with damage dealt and death flags.
[[nodiscard]] CombatResult resolveCombat(const Unit &attacker, const Unit &defender,
                                         const CombatContext &context = CombatContext{});

} // namespace game
