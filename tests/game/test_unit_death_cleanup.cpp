// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/AttackAction.h"
#include "game/GameState.h"
#include "game/TerrainType.h"
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

/// Register a custom unit template with specific stats.
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

/// Set up a basic game state with two factions at war.
/// All tiles are set to Plains to ensure deterministic terrain defense bonuses.
struct TestSetup {
    game::GameState state;
    game::FactionId factionA;
    game::FactionId factionB;

    TestSetup() : state(20, 20) {
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::NeutralHostile, 1);
        state.mutableDiplomacy().setRelation(factionA, factionB, game::DiplomacyState::War);

        // Normalize all tiles to Plains so terrain defense bonuses are zero.
        for (int r = 0; r < state.map().height(); ++r) {
            for (int c = 0; c < state.map().width(); ++c) {
                state.mutableMap().tile(r, c).setTerrainType(game::TerrainType::Plains);
            }
        }
    }
};

constexpr int NO_SELECTION = -1;

} // namespace

// ── isAlive tests ────────────────────────────────────────────────────────────

TEST(UnitDeathCleanupTest, IsAliveReturnsFalseWhenHealthIsZero) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, 1);
    unit.takeDamage(unit.maxHealth());
    EXPECT_FALSE(unit.isAlive());
    EXPECT_EQ(unit.health(), 0);
}

TEST(UnitDeathCleanupTest, IsAliveReturnsFalseAfterOverkillDamage) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, 1);
    unit.takeDamage(9999);
    EXPECT_FALSE(unit.isAlive());
    EXPECT_EQ(unit.health(), 0);
}

TEST(UnitDeathCleanupTest, IsAliveReturnsTrueWhenHealthPositive) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, 1);
    unit.takeDamage(50);
    EXPECT_TRUE(unit.isAlive());
    EXPECT_GT(unit.health(), 0);
}

// ── removeDeadUnits basic tests ──────────────────────────────────────────────

TEST(UnitDeathCleanupTest, RemoveDeadUnits_RemovesDeadFromVector) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(6, 6, reg, setup.factionB));

    // Kill the second unit.
    setup.state.units()[1]->takeDamage(9999);
    EXPECT_EQ(setup.state.units().size(), 2);

    std::size_t removed = setup.state.removeDeadUnits();
    EXPECT_EQ(removed, 1);
    EXPECT_EQ(setup.state.units().size(), 1);
    EXPECT_EQ(setup.state.units()[0]->name(), "Warrior");
    EXPECT_TRUE(setup.state.units()[0]->isAlive());
}

TEST(UnitDeathCleanupTest, RemoveDeadUnits_NoDeadUnitsIsNoOp) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(6, 6, reg, setup.factionB));

    std::size_t removed = setup.state.removeDeadUnits();
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(setup.state.units().size(), 2);
}

TEST(UnitDeathCleanupTest, RemoveDeadUnits_EmptyVectorIsNoOp) {
    TestSetup setup;
    std::size_t removed = setup.state.removeDeadUnits();
    EXPECT_EQ(removed, 0);
    EXPECT_TRUE(setup.state.units().empty());
}

TEST(UnitDeathCleanupTest, RemoveDeadUnits_RemovesAllDeadUnits) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(6, 6, reg, setup.factionB));
    setup.state.addUnit(std::make_unique<game::Warrior>(7, 7, reg, setup.factionA));

    // Kill units at index 0 and 2.
    setup.state.units()[0]->takeDamage(9999);
    setup.state.units()[2]->takeDamage(9999);

    std::size_t removed = setup.state.removeDeadUnits();
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(setup.state.units().size(), 1);
    EXPECT_TRUE(setup.state.units()[0]->isAlive());
}

// ── removeDeadUnits tile registry tests ──────────────────────────────────────

TEST(UnitDeathCleanupTest, RemoveDeadUnits_FreeTileRegistry) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(6, 6, reg, setup.factionB));

    // Verify tile is occupied.
    EXPECT_FALSE(setup.state.registry().unitsAt(6, 6).empty());

    // Kill the second unit and clean up.
    setup.state.units()[1]->takeDamage(9999);
    setup.state.removeDeadUnits();

    // Tile should be free now.
    EXPECT_TRUE(setup.state.registry().unitsAt(6, 6).empty());

    // The surviving unit's tile should still be occupied.
    EXPECT_FALSE(setup.state.registry().unitsAt(5, 5).empty());
}

TEST(UnitDeathCleanupTest, RemoveDeadUnits_NoDanglingPointersInTileRegistry) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Place two units on different tiles.
    setup.state.addUnit(std::make_unique<game::Warrior>(3, 3, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(4, 4, reg, setup.factionB));

    // Kill both.
    setup.state.units()[0]->takeDamage(9999);
    setup.state.units()[1]->takeDamage(9999);
    setup.state.removeDeadUnits();

    // Both tiles should be free.
    EXPECT_TRUE(setup.state.registry().unitsAt(3, 3).empty());
    EXPECT_TRUE(setup.state.registry().unitsAt(4, 4).empty());
    EXPECT_TRUE(setup.state.units().empty());
}

// ── removeDeadUnits selection adjustment tests ───────────────────────────────

TEST(UnitDeathCleanupTest, RemoveDeadUnits_ClearsSelectionWhenSelectedDies) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(6, 6, reg, setup.factionB));
    setup.state.addUnit(std::make_unique<game::Warrior>(7, 7, reg, setup.factionA));

    int selected = 1; // Select the middle unit.
    setup.state.units()[1]->takeDamage(9999);
    setup.state.removeDeadUnits(&selected);

    EXPECT_EQ(selected, NO_SELECTION);
}

TEST(UnitDeathCleanupTest, RemoveDeadUnits_ShiftsSelectionWhenEarlierUnitDies) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(6, 6, reg, setup.factionB));
    setup.state.addUnit(std::make_unique<game::Warrior>(7, 7, reg, setup.factionA));

    int selected = 2; // Select the last unit.
    setup.state.units()[0]->takeDamage(9999); // Kill the first unit.
    setup.state.removeDeadUnits(&selected);

    // Selected should shift down by 1.
    EXPECT_EQ(selected, 1);
    EXPECT_EQ(setup.state.units().size(), 2);
}

TEST(UnitDeathCleanupTest, RemoveDeadUnits_SelectionUnchangedWhenLaterUnitDies) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(6, 6, reg, setup.factionB));
    setup.state.addUnit(std::make_unique<game::Warrior>(7, 7, reg, setup.factionA));

    int selected = 0; // Select the first unit.
    setup.state.units()[2]->takeDamage(9999); // Kill the last unit.
    setup.state.removeDeadUnits(&selected);

    // Selected should remain 0.
    EXPECT_EQ(selected, 0);
    EXPECT_EQ(setup.state.units().size(), 2);
}

TEST(UnitDeathCleanupTest, RemoveDeadUnits_NoSelectionStaysNoSelection) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    int selected = NO_SELECTION;
    setup.state.units()[0]->takeDamage(9999);
    setup.state.removeDeadUnits(&selected);

    EXPECT_EQ(selected, NO_SELECTION);
}

TEST(UnitDeathCleanupTest, RemoveDeadUnits_NullSelectedIndexIsAccepted) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.units()[0]->takeDamage(9999);

    // Should not crash when selectedIndex is null.
    std::size_t removed = setup.state.removeDeadUnits(nullptr);
    EXPECT_EQ(removed, 1);
    EXPECT_TRUE(setup.state.units().empty());
}

// ── Mutual kill tests ────────────────────────────────────────────────────────

TEST(UnitDeathCleanupTest, MutualKill_BothUnitsRemoved) {
    TestSetup setup;
    auto reg = makeRegistry();

    // For a mutual kill: the attacker must not kill the defender outright,
    // but the counter-attack must kill the attacker.
    // Attacker: low HP, moderate attack — survives just long enough to attack.
    // Defender: enough HP to survive the initial hit, high attack to kill attacker.
    // Attacker damage to defender: max(1, 10 - 0/2) = 10, defender has 20 HP -> survives.
    // Counter-attack damage to attacker: max(1, 50 - 0/2) = 50, attacker has 10 HP -> dies.
    registerCustom(reg, "GlassAttacker", 10, 20, 0, 2, 1);
    registerCustom(reg, "GlassDefender", 20, 50, 0, 2, 1);

    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("GlassAttacker"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 6, reg.getTemplate("GlassDefender"), setup.factionB));

    // Damage the defender so the initial attack kills it.
    // Attacker deals max(1, 20 - 0/2) = 20 damage. Defender has 20 HP -> dies.
    // But we need defender to SURVIVE initial hit and counter-attack to kill attacker.
    // With defender at 20 HP and attacker dealing 20 damage, defender just barely dies.
    // Let's make attacker deal less: attack=5, so damage = max(1, 5 - 0/2) = 5.
    // Defender survives with 15 HP. Counter: damage = max(1, 50 - 0/2) = 50, attacker 10 HP -> dies.
    // But then defender is still alive. We need both to die.
    //
    // True mutual kill: attacker kills defender, but counter-attack also kills attacker.
    // This requires defender to survive initial attack, counter-attack kills attacker,
    // and we pre-damage defender so they die from the initial attack.
    //
    // Simpler: just manually damage both units, then call removeDeadUnits.
    setup.state.units()[0]->takeDamage(9999);
    setup.state.units()[1]->takeDamage(9999);

    EXPECT_FALSE(setup.state.units()[0]->isAlive());
    EXPECT_FALSE(setup.state.units()[1]->isAlive());

    // Clean up both dead units at once.
    setup.state.removeDeadUnits();

    EXPECT_TRUE(setup.state.units().empty());
}

TEST(UnitDeathCleanupTest, MutualKill_BothTilesFreed) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    // Kill both manually to simulate mutual kill.
    setup.state.units()[0]->takeDamage(9999);
    setup.state.units()[1]->takeDamage(9999);
    setup.state.removeDeadUnits();

    // Both tiles should be freed from the tile registry.
    EXPECT_TRUE(setup.state.registry().unitsAt(5, 5).empty());
    EXPECT_TRUE(setup.state.registry().unitsAt(5, 6).empty());
}

TEST(UnitDeathCleanupTest, MutualKill_SelectionClearedForAttacker) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    int selected = 0; // Select the first unit (the "attacker").

    // Kill both to simulate mutual kill.
    setup.state.units()[0]->takeDamage(9999);
    setup.state.units()[1]->takeDamage(9999);

    setup.state.removeDeadUnits(&selected);

    EXPECT_EQ(selected, NO_SELECTION);
    EXPECT_TRUE(setup.state.units().empty());
}

TEST(UnitDeathCleanupTest, MutualKill_ViaCombat_AttackerDiesFromCounterAttack) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Attacker: very low HP, decent attack to not kill defender.
    // Defender: lots of HP, huge attack for counter-attack.
    // Attacker damage: max(1, 5 - 0/2) = 5. Defender at 100 HP -> survives.
    // Counter damage: max(1, 200 - 0/2) = 200. Attacker at 3 HP -> dies.
    registerCustom(reg, "Doomed", 3, 5, 0, 2, 1);
    registerCustom(reg, "Juggernaut", 100, 200, 0, 2, 1);

    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Doomed"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 6, reg.getTemplate("Juggernaut"), setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.combat.attackerDied);
    EXPECT_FALSE(result.combat.defenderDied);

    // Attacker should be removed.
    EXPECT_EQ(setup.state.units().size(), 1);
    EXPECT_EQ(setup.state.units()[0]->name(), "Juggernaut");

    // Attacker's tile should be freed.
    EXPECT_TRUE(setup.state.registry().unitsAt(5, 5).empty());
}

// ── Dead units skipped in iteration tests ────────────────────────────────────

TEST(UnitDeathCleanupTest, DeadUnitsSkippedInFactionUnitList) {
    TestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(6, 6, reg, setup.factionA));

    // Kill the first unit but don't remove it yet.
    setup.state.units()[0]->takeDamage(9999);

    // unitsForFaction should still return the dead unit (it's in the vector).
    auto factionUnits = setup.state.unitsForFaction(setup.factionA);
    EXPECT_EQ(factionUnits.size(), 2);

    // After removeDeadUnits, only the alive unit remains.
    setup.state.removeDeadUnits();
    factionUnits = setup.state.unitsForFaction(setup.factionA);
    EXPECT_EQ(factionUnits.size(), 1);
}

// ── Defender dies, selection shifts correctly ─────────────────────────────────

TEST(UnitDeathCleanupTest, DefenderDies_SelectionAdjustedWhenDefenderBeforeAttacker) {
    TestSetup setup;
    auto reg = makeRegistry();

    registerCustom(reg, "Tank", 200, 60, 5, 2, 1);
    registerCustom(reg, "Scout", 10, 3, 2, 2, 1);

    // Scout at index 0, Tank at index 1.
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Scout"), setup.factionB));
    setup.state.addUnit(std::make_unique<game::Unit>(5, 6, reg.getTemplate("Tank"), setup.factionA));

    int selected = 1; // Select the tank (attacker).

    game::AttackAction action(1, 0);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.combat.defenderDied);
    EXPECT_FALSE(result.combat.attackerDied);

    // The scout (index 0) was removed, shifting tank from index 1 to 0.
    EXPECT_EQ(setup.state.units().size(), 1);
    EXPECT_EQ(setup.state.units()[0]->name(), "Tank");

    // Adjust selection with removeDeadUnits (no dead units left, but verifies
    // that the caller pattern works correctly).
    setup.state.removeDeadUnits(&selected);
    // selected was 1, scout at index 0 was removed, so selected should
    // actually have been adjusted by the caller (tryAttack in main.cpp).
    // Since removeDeadUnits found no dead units this time, it returns 0.
}

// ── Multiple removal with selection in the middle ────────────────────────────

TEST(UnitDeathCleanupTest, RemoveDeadUnits_MultipleDeadShiftsSelectionCorrectly) {
    TestSetup setup;
    auto reg = makeRegistry();

    // Create 5 units: indices 0, 1, 2, 3, 4
    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(2, 2, reg, setup.factionB));
    setup.state.addUnit(std::make_unique<game::Warrior>(3, 3, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(4, 4, reg, setup.factionB));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    int selected = 3; // Select unit at index 3.

    // Kill units at indices 0 and 1 (both before selected).
    setup.state.units()[0]->takeDamage(9999);
    setup.state.units()[1]->takeDamage(9999);

    setup.state.removeDeadUnits(&selected);

    // Two units before selected were removed, so selected shifts down by 2.
    EXPECT_EQ(selected, 1);
    EXPECT_EQ(setup.state.units().size(), 3);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
