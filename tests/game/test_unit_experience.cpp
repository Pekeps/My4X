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

constexpr game::FactionId FACTION_A = 1;
constexpr game::FactionId FACTION_B = 2;

/// Build a registry with the default unit types.
game::UnitTypeRegistry makeRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

/// Register a custom unit template for testing specific stat combinations.
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

// ── Basic experience and level tests ─────────────────────────────────────────

TEST(UnitExperienceTest, InitialStateIsZero) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);
    EXPECT_EQ(unit.experience(), 0);
    EXPECT_EQ(unit.level(), 0);
    EXPECT_FALSE(unit.justLeveledUp());
}

TEST(UnitExperienceTest, AddExperienceAccumulates) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);
    unit.addExperience(30);
    EXPECT_EQ(unit.experience(), 30);
    unit.addExperience(20);
    EXPECT_EQ(unit.experience(), 50);
}

TEST(UnitExperienceTest, AddZeroOrNegativeXpDoesNothing) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);
    EXPECT_FALSE(unit.addExperience(0));
    EXPECT_FALSE(unit.addExperience(-10));
    EXPECT_EQ(unit.experience(), 0);
}

TEST(UnitExperienceTest, LevelUpAtThreshold) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);

    // XP_THRESHOLDS[0] = 100 for level 1
    EXPECT_FALSE(unit.addExperience(99));
    EXPECT_EQ(unit.level(), 0);

    EXPECT_TRUE(unit.addExperience(1)); // Now at 100 XP exactly
    EXPECT_EQ(unit.level(), 1);
    EXPECT_TRUE(unit.justLeveledUp());
}

TEST(UnitExperienceTest, LevelUpMultipleLevelsAtOnce) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);

    // XP_THRESHOLDS = {100, 250, 500, 750, 1000}
    // Adding 500 XP should jump to level 3
    EXPECT_TRUE(unit.addExperience(500));
    EXPECT_EQ(unit.level(), 3);
    EXPECT_TRUE(unit.justLeveledUp());
}

TEST(UnitExperienceTest, MaxLevelCap) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);

    // MAX_UNIT_LEVEL = 5, XP_THRESHOLDS[4] = 1000
    unit.addExperience(2000); // Way past max
    EXPECT_EQ(unit.level(), game::MAX_UNIT_LEVEL);
    EXPECT_EQ(unit.experience(), 2000); // XP still accumulates
}

TEST(UnitExperienceTest, AllLevelThresholds) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);

    // Level 1 at 100 XP
    unit.addExperience(100);
    EXPECT_EQ(unit.level(), 1);

    // Level 2 at 250 XP
    unit.addExperience(150);
    EXPECT_EQ(unit.level(), 2);

    // Level 3 at 500 XP
    unit.addExperience(250);
    EXPECT_EQ(unit.level(), 3);

    // Level 4 at 750 XP
    unit.addExperience(250);
    EXPECT_EQ(unit.level(), 4);

    // Level 5 at 1000 XP
    unit.addExperience(250);
    EXPECT_EQ(unit.level(), 5);
}

// ── Stat bonus tests ─────────────────────────────────────────────────────────

TEST(UnitExperienceTest, StatBonusPerLevel) {
    auto reg = makeRegistry();
    // Warrior: attack=15, defense=10
    game::Warrior unit(0, 0, reg, FACTION_A);

    EXPECT_EQ(unit.attack(), 15);
    EXPECT_EQ(unit.defense(), 10);
    EXPECT_EQ(unit.baseAttack(), 15);
    EXPECT_EQ(unit.baseDefense(), 10);

    // Level up to 1: +2 each
    unit.addExperience(100);
    EXPECT_EQ(unit.level(), 1);
    EXPECT_EQ(unit.attack(), 17);  // 15 + 2
    EXPECT_EQ(unit.defense(), 12); // 10 + 2
    EXPECT_EQ(unit.baseAttack(), 15);
    EXPECT_EQ(unit.baseDefense(), 10);

    // Level up to 3: +6 each
    unit.addExperience(400);
    EXPECT_EQ(unit.level(), 3);
    EXPECT_EQ(unit.attack(), 21);  // 15 + 6
    EXPECT_EQ(unit.defense(), 16); // 10 + 6
}

TEST(UnitExperienceTest, MaxLevelStatBonus) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);

    // Max level = 5, bonus per level = 2, so +10 each at max
    unit.addExperience(1000);
    EXPECT_EQ(unit.level(), 5);
    EXPECT_EQ(unit.attack(), 25);  // 15 + 10
    EXPECT_EQ(unit.defense(), 20); // 10 + 10
}

// ── Level-up flag tests ──────────────────────────────────────────────────────

TEST(UnitExperienceTest, LevelUpFlagSetAndCleared) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);

    EXPECT_FALSE(unit.justLeveledUp());

    unit.addExperience(100); // Level up to 1
    EXPECT_TRUE(unit.justLeveledUp());

    unit.clearLevelUpFlag();
    EXPECT_FALSE(unit.justLeveledUp());
}

TEST(UnitExperienceTest, NoLevelUpFlagIfNoLevelChange) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);

    unit.addExperience(50); // Not enough for level 1
    EXPECT_FALSE(unit.justLeveledUp());
}

// ── setExperience tests ──────────────────────────────────────────────────────

TEST(UnitExperienceTest, SetExperienceRecomputesLevel) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);

    unit.setExperience(500);
    EXPECT_EQ(unit.experience(), 500);
    EXPECT_EQ(unit.level(), 3);
}

TEST(UnitExperienceTest, SetExperienceNegativeClampsToZero) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, FACTION_A);
    unit.addExperience(200);

    unit.setExperience(-50);
    EXPECT_EQ(unit.experience(), 0);
    EXPECT_EQ(unit.level(), 0);
}

// ── Combat XP integration tests ──────────────────────────────────────────────

TEST(UnitExperienceTest, AttackerGainsXpFromDamageDealt) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Warrior vs Warrior: damage = 10 each way (melee counter-attack)
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);

    // Attacker XP: damage dealt * XP_PER_DAMAGE_DEALT = 10 * 3 = 30
    // No kill bonus since defender survived.
    EXPECT_EQ(setup.state.units()[0]->experience(), 30);

    // Defender XP: XP_DEFEND_SURVIVE (10) + counter-attack damage * 3 = 10 + 30 = 40
    EXPECT_EQ(setup.state.units()[1]->experience(), 40);
}

TEST(UnitExperienceTest, AttackerGetsKillBonus) {
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

    // Attacker XP: damage * 3 + kill bonus = 10 * 3 + 50 = 80
    // (damage clamped to defender HP = 10)
    EXPECT_EQ(setup.state.units()[0]->experience(), 80);
}

TEST(UnitExperienceTest, DefenderGainsXpForSurviving) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Use an archer as defender (no counter-attack)
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 6, reg.getTemplate("Archer"), setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);
    EXPECT_FALSE(result.combat.defenderDied);
    EXPECT_EQ(result.combat.damageToAttacker, 0); // Archer doesn't counter-attack

    // Defender XP: XP_DEFEND_SURVIVE = 10 (no counter damage dealt)
    EXPECT_EQ(setup.state.units()[1]->experience(), 10);
}

TEST(UnitExperienceTest, DeadAttackerGetsNoXp) {
    TestSetup setup;
    auto reg = makeRegistry();
    registerCustom(reg, "WeakUnit", 5, 10, 2, 2, 1);
    registerCustom(reg, "StrongUnit", 200, 50, 8, 2, 1);

    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("WeakUnit"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 6, reg.getTemplate("StrongUnit"), setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.combat.attackerDied);

    // Attacker was removed. Defender should have XP though.
    // Defender XP: XP_DEFEND_SURVIVE (10) + counter damage * 3 (5*3=15) + kill bonus (50) = 75
    EXPECT_EQ(setup.state.units()[0]->experience(), 75);
}

TEST(UnitExperienceTest, LeveledUnitHasHigherEffectiveStats) {
    auto reg = makeRegistry();
    // Warrior: attack=15, defense=10
    game::Unit attacker(0, 0, reg.getTemplate("Warrior"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Warrior"), FACTION_B);

    // Give the attacker enough XP for level 2: 250 XP
    attacker.addExperience(250);
    EXPECT_EQ(attacker.level(), 2);
    EXPECT_EQ(attacker.attack(), 19);  // 15 + 2*2
    EXPECT_EQ(attacker.defense(), 14); // 10 + 2*2

    // Combat should use the effective (leveled) stats.
    auto result = game::resolveCombat(attacker, defender);

    // damage = max(1, 19 - 10/2) = max(1, 14) = 14
    EXPECT_EQ(result.damageToDefender, 14);

    // Counter-attack: defender uses base stats.
    // counter = max(1, 15 - 14/2) = max(1, 8) = 8
    EXPECT_EQ(result.damageToAttacker, 8);
}

TEST(UnitExperienceTest, RangedAttackXpWithNoCounterAttack) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Archer (range 3) attacks warrior from distance 3.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    EXPECT_TRUE(result.executed);

    // Archer damage to warrior: max(1, 12 - 10/2) = max(1, 7) = 7
    // Archer XP: 7 * 3 = 21 (no kill)
    EXPECT_EQ(setup.state.units()[0]->experience(), 21);

    // Warrior XP: XP_DEFEND_SURVIVE = 10 (no counter damage since ranged)
    EXPECT_EQ(setup.state.units()[1]->experience(), 10);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
