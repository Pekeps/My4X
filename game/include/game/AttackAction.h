#pragma once

#include "game/Combat.h"
#include "game/DiplomacyManager.h"
#include "game/GameState.h"
#include "game/Unit.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace game {

/// Cost in movement points to perform an attack action.
static constexpr int ATTACK_MOVEMENT_COST = 1;

/// Melee range: attacker must be adjacent (distance 1) to the target.
static constexpr int MELEE_RANGE = 1;

/// Validation result for an attack action.
enum class AttackValidation : std::uint8_t {
    Valid,
    AttackerNotFound,
    TargetNotFound,
    AttackerDead,
    TargetDead,
    SameFaction,
    NotAtWar,
    OutOfRange,
    NoMovementRemaining,
};

/// Encapsulates the result of executing an attack action, combining the
/// combat result with post-combat state information.
struct AttackResult {
    CombatResult combat;
    bool executed = false;
    AttackValidation validation = AttackValidation::Valid;
};

/// Player-facing attack action: a unit attacks an enemy unit with immediate
/// combat resolution.
///
/// Usage:
///   1. Construct with attacker and target unit indices.
///   2. Call validate() to check preconditions.
///   3. Call execute() to resolve combat, apply damage, and remove dead units.
///
/// Melee attacks require the target to be adjacent (hex distance 1).
/// Ranged attacks use the attacker's UnitTemplate::attackRange.
/// Counter-attacks only occur for melee combat (see Combat.h).
class AttackAction {
  public:
    /// Construct an attack action from attacker index to target index.
    AttackAction(std::size_t attackerIndex, std::size_t targetIndex);

    /// Validate whether this attack can be executed against the current game state.
    [[nodiscard]] AttackValidation validate(const GameState &state) const;

    /// Execute the attack: resolve combat, apply damage, deduct movement,
    /// and remove dead units from the game.
    ///
    /// Returns an AttackResult describing what happened. If validation fails,
    /// the result will have executed=false and the appropriate validation code.
    [[nodiscard]] AttackResult execute(GameState &state) const;

    /// Compute hex distance between two positions using axial coordinates.
    /// Uses odd-r offset coordinate conversion to cube coordinates.
    [[nodiscard]] static int hexDistance(int row1, int col1, int row2, int col2);

  private:
    std::size_t attackerIndex_;
    std::size_t targetIndex_;
};

} // namespace game
