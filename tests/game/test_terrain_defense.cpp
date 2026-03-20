// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/AttackAction.h"
#include "game/Combat.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/Map.h"
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

/// Set up a basic game state with two factions at war for testing.
struct TerrainDefenseSetup {
    game::GameState state;
    game::FactionId factionA;
    game::FactionId factionB;

    TerrainDefenseSetup() : state(20, 20) {
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::NeutralHostile, 1);

        // Set factions at war.
        state.mutableDiplomacy().setRelation(factionA, factionB, game::DiplomacyState::War);
    }

    /// Place the defender on a tile with the given terrain type.
    void setDefenderTerrain(int row, int col, game::TerrainType terrain) {
        state.mutableMap().tile(row, col).setTerrainType(terrain);
    }

    /// Place the attacker on a tile with the given terrain type.
    void setAttackerTerrain(int row, int col, game::TerrainType terrain) {
        state.mutableMap().tile(row, col).setTerrainType(terrain);
    }
};

} // namespace

// ── Forest defender takes less damage than plains defender ───────────────────

TEST(TerrainDefenseTest, ForestDefender_TakesLessDamage) {
    auto reg = makeRegistry();

    // Combat on plains (baseline).
    TerrainDefenseSetup plainsSetup;
    plainsSetup.setDefenderTerrain(5, 6, game::TerrainType::Plains);
    plainsSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    plainsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, plainsSetup.factionA));
    plainsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, plainsSetup.factionB));

    game::AttackAction plainsAction(0, 1);
    auto plainsResult = plainsAction.execute(plainsSetup.state);
    ASSERT_TRUE(plainsResult.executed);

    // Combat on forest.
    TerrainDefenseSetup forestSetup;
    forestSetup.setDefenderTerrain(5, 6, game::TerrainType::Forest);
    forestSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    forestSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, forestSetup.factionA));
    forestSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, forestSetup.factionB));

    game::AttackAction forestAction(0, 1);
    auto forestResult = forestAction.execute(forestSetup.state);
    ASSERT_TRUE(forestResult.executed);

    // Forest defender should take less damage than plains defender.
    EXPECT_LT(forestResult.combat.damageToDefender, plainsResult.combat.damageToDefender);
}

// ── Hills defender takes less damage than plains defender ────────────────────

TEST(TerrainDefenseTest, HillsDefender_TakesLessDamage) {
    auto reg = makeRegistry();

    // Combat on plains (baseline).
    TerrainDefenseSetup plainsSetup;
    plainsSetup.setDefenderTerrain(5, 6, game::TerrainType::Plains);
    plainsSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    plainsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, plainsSetup.factionA));
    plainsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, plainsSetup.factionB));

    game::AttackAction plainsAction(0, 1);
    auto plainsResult = plainsAction.execute(plainsSetup.state);
    ASSERT_TRUE(plainsResult.executed);

    // Combat on hills.
    TerrainDefenseSetup hillsSetup;
    hillsSetup.setDefenderTerrain(5, 6, game::TerrainType::Hills);
    hillsSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    hillsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, hillsSetup.factionA));
    hillsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, hillsSetup.factionB));

    game::AttackAction hillsAction(0, 1);
    auto hillsResult = hillsAction.execute(hillsSetup.state);
    ASSERT_TRUE(hillsResult.executed);

    // Hills defender should take less damage than plains defender.
    EXPECT_LT(hillsResult.combat.damageToDefender, plainsResult.combat.damageToDefender);
}

// ── Hills defender takes less damage than forest defender ────────────────────

TEST(TerrainDefenseTest, HillsDefender_TakesLessOrEqualThanForest) {
    auto reg = makeRegistry();

    // Combat on forest (defense +2).
    TerrainDefenseSetup forestSetup;
    forestSetup.setDefenderTerrain(5, 6, game::TerrainType::Forest);
    forestSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    forestSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, forestSetup.factionA));
    forestSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, forestSetup.factionB));

    game::AttackAction forestAction(0, 1);
    auto forestResult = forestAction.execute(forestSetup.state);
    ASSERT_TRUE(forestResult.executed);

    // Combat on hills (defense +3).
    TerrainDefenseSetup hillsSetup;
    hillsSetup.setDefenderTerrain(5, 6, game::TerrainType::Hills);
    hillsSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    hillsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, hillsSetup.factionA));
    hillsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, hillsSetup.factionB));

    game::AttackAction hillsAction(0, 1);
    auto hillsResult = hillsAction.execute(hillsSetup.state);
    ASSERT_TRUE(hillsResult.executed);

    // Hills (+3) gives at least as much protection as forest (+2).
    // Due to integer division in the damage formula, the difference may be zero
    // for certain stat combinations (e.g. 12/2 == 13/2 == 6).
    EXPECT_LE(hillsResult.combat.damageToDefender, forestResult.combat.damageToDefender);
}

// ── Swamp defender takes more damage (negative bonus) ───────────────────────

TEST(TerrainDefenseTest, SwampDefender_TakesMoreDamage) {
    auto reg = makeRegistry();

    // Combat on plains (baseline, defense +0).
    TerrainDefenseSetup plainsSetup;
    plainsSetup.setDefenderTerrain(5, 6, game::TerrainType::Plains);
    plainsSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    plainsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, plainsSetup.factionA));
    plainsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, plainsSetup.factionB));

    game::AttackAction plainsAction(0, 1);
    auto plainsResult = plainsAction.execute(plainsSetup.state);
    ASSERT_TRUE(plainsResult.executed);

    // Combat on swamp (defense -1).
    TerrainDefenseSetup swampSetup;
    swampSetup.setDefenderTerrain(5, 6, game::TerrainType::Swamp);
    swampSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    swampSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, swampSetup.factionA));
    swampSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, swampSetup.factionB));

    game::AttackAction swampAction(0, 1);
    auto swampResult = swampAction.execute(swampSetup.state);
    ASSERT_TRUE(swampResult.executed);

    // Swamp defender should take more damage than plains defender.
    EXPECT_GT(swampResult.combat.damageToDefender, plainsResult.combat.damageToDefender);
}

// ── Exact damage values with terrain defense ────────────────────────────────

TEST(TerrainDefenseTest, ForestDefender_ExactDamage) {
    auto reg = makeRegistry();

    // Warrior: attack=15, defense=10
    // On forest: effective defense = 10 + 2 = 12
    // Damage = max(1, 15 - 12/2) = max(1, 9) = 9
    TerrainDefenseSetup setup;
    setup.setDefenderTerrain(5, 6, game::TerrainType::Forest);
    setup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    ASSERT_TRUE(result.executed);

    EXPECT_EQ(result.combat.damageToDefender, 9);
}

TEST(TerrainDefenseTest, HillsDefender_ExactDamage) {
    auto reg = makeRegistry();

    // Warrior: attack=15, defense=10
    // On hills: effective defense = 10 + 3 = 13
    // Damage = max(1, 15 - 13/2) = max(1, 15 - 6) = 9
    // (Integer division: 13/2 = 6)
    TerrainDefenseSetup setup;
    setup.setDefenderTerrain(5, 6, game::TerrainType::Hills);
    setup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    ASSERT_TRUE(result.executed);

    // effectiveDefense = 10 + 3 = 13, 13/2 = 6, damage = 15 - 6 = 9
    EXPECT_EQ(result.combat.damageToDefender, 9);
}

TEST(TerrainDefenseTest, PlainsDefender_ExactDamage) {
    auto reg = makeRegistry();

    // Warrior: attack=15, defense=10
    // On plains: effective defense = 10 + 0 = 10
    // Damage = max(1, 15 - 10/2) = max(1, 10) = 10
    TerrainDefenseSetup setup;
    setup.setDefenderTerrain(5, 6, game::TerrainType::Plains);
    setup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    ASSERT_TRUE(result.executed);

    EXPECT_EQ(result.combat.damageToDefender, 10);
}

// ── Attacker terrain bonus reduces counter-attack damage ────────────────────

TEST(TerrainDefenseTest, AttackerOnForest_ReducesCounterDamage) {
    auto reg = makeRegistry();

    // Attacker on forest (defense +2), defender on plains.
    // Counter-attack: defender attacks with 15, attacker defense = 10 + 2 = 12
    // Counter damage = max(1, 15 - 12/2) = max(1, 9) = 9
    TerrainDefenseSetup setup;
    setup.setDefenderTerrain(5, 6, game::TerrainType::Plains);
    setup.setAttackerTerrain(5, 5, game::TerrainType::Forest);
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    ASSERT_TRUE(result.executed);

    // Counter-attack damage should be reduced due to attacker's forest terrain.
    EXPECT_EQ(result.combat.damageToAttacker, 9);
    // Primary attack should be unmodified (defender on plains).
    EXPECT_EQ(result.combat.damageToDefender, 10);
}

// ── Desert terrain has no defense bonus (same as plains) ────────────────────

TEST(TerrainDefenseTest, DesertDefender_SameAsPlains) {
    auto reg = makeRegistry();

    // Plains baseline.
    TerrainDefenseSetup plainsSetup;
    plainsSetup.setDefenderTerrain(5, 6, game::TerrainType::Plains);
    plainsSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    plainsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, plainsSetup.factionA));
    plainsSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, plainsSetup.factionB));

    game::AttackAction plainsAction(0, 1);
    auto plainsResult = plainsAction.execute(plainsSetup.state);
    ASSERT_TRUE(plainsResult.executed);

    // Desert should behave identically to plains (defense +0).
    TerrainDefenseSetup desertSetup;
    desertSetup.setDefenderTerrain(5, 6, game::TerrainType::Desert);
    desertSetup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    desertSetup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, desertSetup.factionA));
    desertSetup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, desertSetup.factionB));

    game::AttackAction desertAction(0, 1);
    auto desertResult = desertAction.execute(desertSetup.state);
    ASSERT_TRUE(desertResult.executed);

    EXPECT_EQ(desertResult.combat.damageToDefender, plainsResult.combat.damageToDefender);
}

// ── Both units on defensive terrain ─────────────────────────────────────────

TEST(TerrainDefenseTest, BothOnDefensiveTerrain) {
    auto reg = makeRegistry();

    // Attacker on hills (+3), defender on forest (+2).
    // Primary: defender effective defense = 10 + 2 = 12, damage = max(1, 15 - 12/2) = 9
    // Counter: attacker effective defense = 10 + 3 = 13, counter = max(1, 15 - 13/2) = 9
    TerrainDefenseSetup setup;
    setup.setAttackerTerrain(5, 5, game::TerrainType::Hills);
    setup.setDefenderTerrain(5, 6, game::TerrainType::Forest);
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    ASSERT_TRUE(result.executed);

    EXPECT_EQ(result.combat.damageToDefender, 9);
    EXPECT_EQ(result.combat.damageToAttacker, 9);
}

// ── Terrain defense with ranged attack ──────────────────────────────────────

TEST(TerrainDefenseTest, RangedAttack_TerrainStillApplies) {
    auto reg = makeRegistry();

    // Archer attacks warrior on hills from range.
    // Archer: attack=12, Warrior: defense=10
    // Hills defense bonus = +3, effective defense = 13
    // Damage = max(1, 12 - 13/2) = max(1, 6) = 6
    TerrainDefenseSetup setup;
    setup.setDefenderTerrain(5, 8, game::TerrainType::Hills);
    setup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    setup.state.addUnit(std::make_unique<game::Unit>(5, 5, reg.getTemplate("Archer"), setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 8, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    ASSERT_TRUE(result.executed);

    EXPECT_EQ(result.combat.damageToDefender, 6);
    // No counter-attack from ranged attack.
    EXPECT_EQ(result.combat.damageToAttacker, 0);
}

// ── Swamp defender exact damage value ───────────────────────────────────────

TEST(TerrainDefenseTest, SwampDefender_ExactDamage) {
    auto reg = makeRegistry();

    // Warrior: attack=15, defense=10
    // On swamp: effective defense = 10 + (-1) = 9
    // Damage = max(1, 15 - 9/2) = max(1, 15 - 4) = 11
    TerrainDefenseSetup setup;
    setup.setDefenderTerrain(5, 6, game::TerrainType::Swamp);
    setup.setAttackerTerrain(5, 5, game::TerrainType::Plains);
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::AttackAction action(0, 1);
    auto result = action.execute(setup.state);
    ASSERT_TRUE(result.executed);

    EXPECT_EQ(result.combat.damageToDefender, 11);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
