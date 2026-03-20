// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/AttackAction.h"
#include "game/Combat.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

#include <memory>

namespace {

/// Build a registry with the default unit types.
game::UnitTypeRegistry makeRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

/// Register a custom unit template.
void registerCustom(game::UnitTypeRegistry &reg, const std::string &name, int maxHealth, int attack, int defense,
                    int movementRange, int attackRange) {
    game::UnitTemplate tmpl;
    tmpl.name = name;
    tmpl.maxHealth = maxHealth;
    tmpl.attack = attack;
    tmpl.defense = defense;
    tmpl.movementRange = movementRange;
    tmpl.attackRange = attackRange;
    reg.registerTemplate(name, tmpl);
}

/// Set up a basic game state with two factions at war for testing.
struct TestSetup {
    game::GameState state;
    game::FactionId factionA;
    game::FactionId factionB;

    TestSetup() : state(20, 20) {
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::NeutralHostile, 1);

        // Set factions at war.
        state.mutableDiplomacy().setRelation(factionA, factionB, game::DiplomacyState::War);
    }
};

} // namespace

// ── Hex distance tests ──────────────────────────────────────────────────────

TEST(AttackActionTest, HexDistance_SameTile) {
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 0, 0), 0);
    EXPECT_EQ(game::AttackAction::hexDistance(5, 3, 5, 3), 0);
}

TEST(AttackActionTest, HexDistance_Adjacent) {
    // Even row neighbors.
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 0, 1), 1);
    EXPECT_EQ(game::AttackAction::hexDistance(0, 1, 0, 0), 1);
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 1, 0), 1);

    // Odd row neighbors.
    EXPECT_EQ(game::AttackAction::hexDistance(1, 0, 1, 1), 1);
    EXPECT_EQ(game::AttackAction::hexDistance(1, 0, 0, 0), 1);
    EXPECT_EQ(game::AttackAction::hexDistance(1, 0, 2, 0), 1);
}

TEST(AttackActionTest, HexDistance_TwoApart) {
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 0, 2), 2);
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 2, 0), 2);
}

TEST(AttackActionTest, HexDistance_ThreeApart) {
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 0, 3), 3);
}

// ── Validation tests ────────────────────────────────────────────────────────

TEST(AttackActionTest, Validate_Valid_MeleeAttack) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Place units adjacent to each other.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::Valid);
}

TEST(AttackActionTest, Validate_AttackerNotFound) {
    TestSetup setup;
    auto reg = makeRegistry();
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::AttackAction action(99, 0);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::AttackerNotFound);
}

TEST(AttackActionTest, Validate_TargetNotFound) {
    TestSetup setup;
    auto reg = makeRegistry();
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::AttackAction action(0, 99);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::TargetNotFound);
}

TEST(AttackActionTest, Validate_AttackerDead) {
    TestSetup setup;
    auto reg = makeRegistry();
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    // Kill the attacker.
    setup.state.units()[0]->takeDamage(999);

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::AttackerDead);
}

TEST(AttackActionTest, Validate_TargetDead) {
    TestSetup setup;
    auto reg = makeRegistry();
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    // Kill the target.
    setup.state.units()[1]->takeDamage(999);

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::TargetDead);
}

TEST(AttackActionTest, Validate_SameFaction) {
    TestSetup setup;
    auto reg = makeRegistry();
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionA));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::SameFaction);
}

TEST(AttackActionTest, Validate_NotAtWar) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Change to peace.
    setup.state.mutableDiplomacy().setRelation(setup.factionA, setup.factionB, game::DiplomacyState::Peace);

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::NotAtWar);
}

TEST(AttackActionTest, Validate_OutOfRange_Melee) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Place units 2 tiles apart (melee range is 1).
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 7, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::OutOfRange);
}

TEST(AttackActionTest, Validate_NoMovementRemaining) {
    TestSetup setup;
    auto reg = makeRegistry();
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    // Exhaust movement.
    setup.state.units()[0]->setMovementRemaining(0);

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::NoMovementRemaining);
}

TEST(AttackActionTest, Validate_RangedAttackInRange) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer has attackRange=3, place target 3 tiles away.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::Valid);
}

TEST(AttackActionTest, Validate_RangedAttackOutOfRange) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer has attackRange=3, place target 4 tiles away.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 9, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::OutOfRange);
}

// ── Execution tests ─────────────────────────────────────────────────────────

TEST(AttackActionTest, Execute_MeleeAttack_BothSurvive) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Warrior vs Warrior: both survive one round.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_EQ(result.validation, game::AttackValidation::Valid);

    // Both should still be alive.
    EXPECT_EQ(setup.state.units().size(), 2);

    // Damage should have been applied.
    // Warrior attack=15, defense=10. damage = max(1, 15-10/2) = 10
    EXPECT_EQ(setup.state.units()[1]->health(), 90); // defender
    EXPECT_EQ(setup.state.units()[0]->health(), 90); // attacker (counter-attack)

    // Movement should have been deducted.
    EXPECT_EQ(setup.state.units()[0]->movementRemaining(), 1); // was 2, now 1
}

TEST(AttackActionTest, Execute_MeleeAttack_DefenderDies) {
    TestSetup setup;
    auto reg = makeRegistry();
    registerCustom(reg, "Tank", 200, 60, 5, 2, 1);
    registerCustom(reg, "Scout", 10, 3, 2, 2, 1);

    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Tank"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 6, reg.getTemplate("Scout"), setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.combat.defenderDied);
    EXPECT_FALSE(result.combat.attackerDied);

    // Defender should be removed.
    EXPECT_EQ(setup.state.units().size(), 1);
    EXPECT_EQ(setup.state.units()[0]->name(), "Tank");
}

TEST(AttackActionTest, Execute_MeleeAttack_AttackerDies) {
    TestSetup setup;
    auto reg = makeRegistry();
    registerCustom(reg, "WeakUnit", 5, 10, 2, 2, 1);
    registerCustom(reg, "StrongUnit", 200, 50, 8, 2, 1);

    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("WeakUnit"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 6, reg.getTemplate("StrongUnit"), setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_FALSE(result.combat.defenderDied);
    EXPECT_TRUE(result.combat.attackerDied);

    // Attacker should be removed.
    EXPECT_EQ(setup.state.units().size(), 1);
    EXPECT_EQ(setup.state.units()[0]->name(), "StrongUnit");
}

TEST(AttackActionTest, Execute_RangedAttack_NoCounterAttack) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer (range 3) attacks warrior from distance 3.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    // Archer attacks from range — no counter-attack.
    EXPECT_EQ(result.combat.damageToAttacker, 0);
    EXPECT_GT(result.combat.damageToDefender, 0);

    // Archer health unchanged.
    EXPECT_EQ(setup.state.units()[0]->health(), 70); // Archer max HP
}

TEST(AttackActionTest, Execute_FailedValidation_NoExecution) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Place units too far apart for melee.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);

    EXPECT_FALSE(result.executed);
    EXPECT_EQ(result.validation, game::AttackValidation::OutOfRange);

    // No damage applied.
    EXPECT_EQ(setup.state.units()[0]->health(), 100);
    EXPECT_EQ(setup.state.units()[1]->health(), 100);
}

TEST(AttackActionTest, Execute_MovementDeducted) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    EXPECT_EQ(setup.state.units()[0]->movementRemaining(), 2);

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);

    EXPECT_EQ(setup.state.units()[0]->movementRemaining(), 1);
}

TEST(AttackActionTest, Execute_CannotAttackTwiceWithOneMovement) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    // Set movement to exactly 1 — enough for one attack.
    setup.state.units()[0]->setMovementRemaining(1);

    game::AttackAction action(0, 1);
    auto result1 = action.execute(setup.state);
    EXPECT_TRUE(result1.executed);

    // Second attack should fail — no movement left.
    auto result2 = action.execute(setup.state);
    EXPECT_FALSE(result2.executed);
    EXPECT_EQ(result2.validation, game::AttackValidation::NoMovementRemaining);
}

TEST(AttackActionTest, Execute_DeadUnitsRemovedFromTileRegistry) {
    TestSetup setup;
    auto reg = makeRegistry();
    registerCustom(reg, "OneHit", 1, 100, 0, 2, 1);

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 6, reg.getTemplate("OneHit"), setup.factionB));

    // Verify target is registered.
    EXPECT_FALSE(setup.state.registry().unitsAt(5, 6).empty());

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);

    // Target should be removed from tile registry.
    EXPECT_TRUE(setup.state.registry().unitsAt(5, 6).empty());
}

// ── Ranged attack tests ──────────────────────────────────────────────────────

TEST(AttackActionTest, RangedAttack_AtDistance2_IsValid) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer (range 3) attacks at distance 2 — well within range.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 7, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::Valid);
}

TEST(AttackActionTest, RangedAttack_AtDistance1_IsMelee) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer attacks adjacent unit — should still be valid but treated as melee
    // (distance == 1 which is <= range, and CombatContext::rangedAttack should be false).
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);

    // At distance 1, this is NOT a ranged attack — counter-attack should occur.
    // Warrior is melee (range 1), so it counter-attacks.
    EXPECT_GT(result.combat.damageToAttacker, 0);
}

TEST(AttackActionTest, RangedAttack_NoCounterFromMeleeDefender) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer attacks warrior from distance 2 (ranged attack), no counter-attack.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 7, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);

    // Ranged attack suppresses counter-attack.
    EXPECT_EQ(result.combat.damageToAttacker, 0);
    EXPECT_GT(result.combat.damageToDefender, 0);
}

TEST(AttackActionTest, RangedAttack_KillsDefender_NoCounterAttack) {
    TestSetup setup;
    auto reg = makeRegistry();
    registerCustom(reg, "StrongArcher", 100, 50, 5, 2, 3);

    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("StrongArcher"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 7, reg, setup.factionB));

    // Weaken defender to ensure death.
    setup.state.units()[1]->takeDamage(95);

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.combat.defenderDied);
    EXPECT_FALSE(result.combat.attackerDied);
    EXPECT_EQ(result.combat.damageToAttacker, 0);

    // Only the attacker should remain.
    EXPECT_EQ(setup.state.units().size(), 1);
    EXPECT_EQ(setup.state.units()[0]->name(), "StrongArcher");
}

TEST(AttackActionTest, RangedAttack_MovementDeducted) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    EXPECT_EQ(setup.state.units()[0]->movementRemaining(), 2);

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);

    // Movement should be deducted for ranged attacks too.
    EXPECT_EQ(setup.state.units()[0]->movementRemaining(), 1);
}

TEST(AttackActionTest, RangedAttack_ExhaustedMovement_Rejected) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    setup.state.units()[0]->setMovementRemaining(0);

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::NoMovementRemaining);
}

TEST(AttackActionTest, RangedAttack_ArcherVsArcher_NoCounter) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Both units are ranged archers at distance 2.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 7, reg.getTemplate("Archer"), setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);

    // Ranged attack: no counter-attack (rangedAttack flag is set by distance > 1).
    // Also, even if context.rangedAttack were false, Archer is ranged so wouldn't counter.
    EXPECT_EQ(result.combat.damageToAttacker, 0);
    EXPECT_GT(result.combat.damageToDefender, 0);
}

TEST(AttackActionTest, MeleeUnit_CannotAttackAtRange2) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Warrior (range 1) tries to attack at distance 2 — should be out of range.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 7, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::OutOfRange);
}

TEST(AttackActionTest, HexDistance_VariousRanges) {
    // Verify hex distances used in range calculations.
    // Same row, different columns on even row.
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 0, 1), 1);
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 0, 2), 2);
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 0, 3), 3);
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 0, 4), 4);

    // Diagonal distances.
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 1, 0), 1);
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 2, 1), 2);
    EXPECT_EQ(game::AttackAction::hexDistance(0, 0, 3, 1), 3);
}

TEST(AttackActionTest, RangedAttack_ExactMaxRange) {
    TestSetup setup;
    auto reg = makeRegistry();
    registerCustom(reg, "Sniper", 60, 20, 3, 2, 4);

    // Place at exactly range 4.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 2, reg.getTemplate("Sniper"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::Valid);
}

TEST(AttackActionTest, RangedAttack_JustBeyondMaxRange) {
    TestSetup setup;
    auto reg = makeRegistry();
    registerCustom(reg, "Sniper", 60, 20, 3, 2, 4);

    // Place at range 5 (one beyond max range 4).
    setup.state.addUnit(std::make_unique<game::Unit>(5, 2, reg.getTemplate("Sniper"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 7, reg, setup.factionB));

    game::AttackAction action(0, 1);
    EXPECT_EQ(action.validate(setup.state), game::AttackValidation::OutOfRange);
}

TEST(AttackActionTest, RangedAttack_DamageFormula) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer: attack=12, Warrior: defense=10
    // Expected damage: max(1, 12 - 10/2) = max(1, 7) = 7
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);
    EXPECT_EQ(result.combat.damageToDefender, 7);
    EXPECT_EQ(result.combat.damageToAttacker, 0);

    // Warrior HP should be reduced.
    EXPECT_EQ(setup.state.units()[1]->health(), 93);
    // Archer HP unchanged.
    EXPECT_EQ(setup.state.units()[0]->health(), 70);
}

TEST(AttackActionTest, RangedAttack_MultipleAttacks_SameTurn) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer with 2 movement can attack twice in one turn.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    game::AttackAction action(0, 1);

    auto result1 = action.execute(setup.state);
    EXPECT_TRUE(result1.executed);
    EXPECT_EQ(setup.state.units()[0]->movementRemaining(), 1);

    auto result2 = action.execute(setup.state);
    EXPECT_TRUE(result2.executed);
    EXPECT_EQ(setup.state.units()[0]->movementRemaining(), 0);

    // Third attack should fail.
    auto result3 = action.execute(setup.state);
    EXPECT_FALSE(result3.executed);
    EXPECT_EQ(result3.validation, game::AttackValidation::NoMovementRemaining);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
