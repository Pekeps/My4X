// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/MoveAction.h"

#include "game/AttackAction.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/TerrainType.h"
#include "game/Unit.h"
#include "game/UnitTemplate.h"
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

/// Create a game state with a player faction and controllable terrain.
struct TerrainMoveSetup {
    game::GameState state;
    game::FactionId factionA;

    TerrainMoveSetup() : state(10, 10) {
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);

        // Set all tiles to Plains by default for a clean baseline.
        for (int r = 0; r < state.map().height(); ++r) {
            for (int c = 0; c < state.map().width(); ++c) {
                state.mutableMap().tile(r, c).setTerrainType(game::TerrainType::Plains);
            }
        }
    }
};

} // namespace

// ── Movement cost varies by terrain type ─────────────────────────────────────

TEST(TerrainMovementTest, MoveToPlainsCostsOne) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    // Place unit at (1,1) on plains
    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));
    auto &unit = *setup.state.units()[0];
    int initialMovement = unit.movementRemaining();

    // Destination (1,2) is plains (cost 1)
    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_EQ(result.movementCostDeducted, game::terrain_constants::PLAINS_MOVEMENT_COST);
    EXPECT_EQ(unit.movementRemaining(), initialMovement - game::terrain_constants::PLAINS_MOVEMENT_COST);
    EXPECT_EQ(unit.row(), 1);
    EXPECT_EQ(unit.col(), 2);
}

TEST(TerrainMovementTest, MoveToHillsCostsTwo) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    // Place unit at (1,1) with extra movement
    game::UnitTemplate tmpl;
    tmpl.name = "Scout";
    tmpl.maxHealth = 50;
    tmpl.attack = 5;
    tmpl.defense = 5;
    tmpl.movementRange = 4;
    tmpl.attackRange = 1;
    game::UnitTypeRegistry customReg;
    customReg.registerTemplate("Scout", tmpl);

    setup.state.addUnit(std::make_unique<game::Unit>(1, 1, customReg.getTemplate("Scout"), setup.factionA));
    auto &unit = *setup.state.units()[0];

    // Set destination to hills
    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Hills);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_EQ(result.movementCostDeducted, game::terrain_constants::HILLS_MOVEMENT_COST);
    EXPECT_EQ(unit.movementRemaining(), 4 - game::terrain_constants::HILLS_MOVEMENT_COST);
}

TEST(TerrainMovementTest, MoveToForestCostsTwo) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    game::UnitTemplate tmpl;
    tmpl.name = "Scout";
    tmpl.maxHealth = 50;
    tmpl.attack = 5;
    tmpl.defense = 5;
    tmpl.movementRange = 4;
    tmpl.attackRange = 1;
    game::UnitTypeRegistry customReg;
    customReg.registerTemplate("Scout", tmpl);

    setup.state.addUnit(std::make_unique<game::Unit>(1, 1, customReg.getTemplate("Scout"), setup.factionA));
    auto &unit = *setup.state.units()[0];

    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Forest);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_EQ(result.movementCostDeducted, game::terrain_constants::FOREST_MOVEMENT_COST);
    EXPECT_EQ(unit.movementRemaining(), 4 - game::terrain_constants::FOREST_MOVEMENT_COST);
}

TEST(TerrainMovementTest, MoveToDesertCostsOne) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));
    auto &unit = *setup.state.units()[0];
    int initialMovement = unit.movementRemaining();

    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Desert);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_EQ(result.movementCostDeducted, game::terrain_constants::DESERT_MOVEMENT_COST);
    EXPECT_EQ(unit.movementRemaining(), initialMovement - game::terrain_constants::DESERT_MOVEMENT_COST);
}

TEST(TerrainMovementTest, MoveToSwampCostsThree) {
    TerrainMoveSetup setup;

    game::UnitTemplate tmpl;
    tmpl.name = "Scout";
    tmpl.maxHealth = 50;
    tmpl.attack = 5;
    tmpl.defense = 5;
    tmpl.movementRange = 5;
    tmpl.attackRange = 1;
    game::UnitTypeRegistry customReg;
    customReg.registerTemplate("Scout", tmpl);

    setup.state.addUnit(std::make_unique<game::Unit>(1, 1, customReg.getTemplate("Scout"), setup.factionA));
    auto &unit = *setup.state.units()[0];

    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Swamp);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_EQ(result.movementCostDeducted, game::terrain_constants::SWAMP_MOVEMENT_COST);
    EXPECT_EQ(unit.movementRemaining(), 5 - game::terrain_constants::SWAMP_MOVEMENT_COST);
}

// ── Impassable terrain blocks movement ───────────────────────────────────────

TEST(TerrainMovementTest, CannotMoveToWater) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));
    auto &unit = *setup.state.units()[0];
    int initialMovement = unit.movementRemaining();

    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Water);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_FALSE(result.executed);
    EXPECT_EQ(result.validation, game::MoveValidation::ImpassableTerrain);
    EXPECT_EQ(unit.movementRemaining(), initialMovement);
    EXPECT_EQ(unit.row(), 1);
    EXPECT_EQ(unit.col(), 1);
}

TEST(TerrainMovementTest, CannotMoveToMountain) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));
    auto &unit = *setup.state.units()[0];
    int initialMovement = unit.movementRemaining();

    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Mountain);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_FALSE(result.executed);
    EXPECT_EQ(result.validation, game::MoveValidation::ImpassableTerrain);
    EXPECT_EQ(unit.movementRemaining(), initialMovement);
    EXPECT_EQ(unit.row(), 1);
    EXPECT_EQ(unit.col(), 1);
}

TEST(TerrainMovementTest, ImpassableBlocksEvenWithHighMovement) {
    TerrainMoveSetup setup;

    game::UnitTemplate tmpl;
    tmpl.name = "FastUnit";
    tmpl.maxHealth = 50;
    tmpl.attack = 5;
    tmpl.defense = 5;
    tmpl.movementRange = 99;
    tmpl.attackRange = 1;
    game::UnitTypeRegistry customReg;
    customReg.registerTemplate("FastUnit", tmpl);

    setup.state.addUnit(std::make_unique<game::Unit>(1, 1, customReg.getTemplate("FastUnit"), setup.factionA));

    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Mountain);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_FALSE(result.executed);
    EXPECT_EQ(result.validation, game::MoveValidation::ImpassableTerrain);
}

// ── Insufficient movement points ─────────────────────────────────────────────

TEST(TerrainMovementTest, InsufficientMovementForHills) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    // Warrior has 2 movement. Move once on plains (cost 1), leaving 1.
    // Then try to move to hills (cost 2) — should fail.
    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));
    auto &unit = *setup.state.units()[0];

    // First move to adjacent plains tile (cost 1).
    game::MoveAction firstMove(0, 1, 2);
    auto firstResult = firstMove.execute(setup.state);
    EXPECT_TRUE(firstResult.executed);
    EXPECT_EQ(unit.movementRemaining(), 1);

    // Set destination to hills (cost 2).
    setup.state.mutableMap().tile(1, 3).setTerrainType(game::TerrainType::Hills);

    // Try to move with only 1 movement remaining to a cost-2 tile.
    game::MoveAction secondMove(0, 1, 3);
    auto secondResult = secondMove.execute(setup.state);

    EXPECT_FALSE(secondResult.executed);
    EXPECT_EQ(secondResult.validation, game::MoveValidation::InsufficientMovement);
    EXPECT_EQ(unit.movementRemaining(), 1);
    EXPECT_EQ(unit.row(), 1);
    EXPECT_EQ(unit.col(), 2);
}

TEST(TerrainMovementTest, InsufficientMovementForSwamp) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    // Warrior has 2 movement, swamp costs 3 — should fail immediately.
    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));

    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Swamp);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_FALSE(result.executed);
    EXPECT_EQ(result.validation, game::MoveValidation::InsufficientMovement);
}

// ── Basic movement validation ────────────────────────────────────────────────

TEST(TerrainMovementTest, NoMovementRemainingBlocked) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));
    auto &unit = *setup.state.units()[0];
    unit.setMovementRemaining(0);

    game::MoveAction action(0, 1, 2);
    auto result = action.execute(setup.state);

    EXPECT_FALSE(result.executed);
    EXPECT_EQ(result.validation, game::MoveValidation::NoMovementRemaining);
}

TEST(TerrainMovementTest, NonAdjacentTileBlocked) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));

    // Try to move to a tile that is distance 2 away.
    game::MoveAction action(0, 3, 3);
    auto result = action.execute(setup.state);

    EXPECT_FALSE(result.executed);
    EXPECT_EQ(result.validation, game::MoveValidation::NotAdjacent);
}

TEST(TerrainMovementTest, ValidateReturnsValidForLegalMove) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));

    game::MoveAction action(0, 1, 2);
    auto validation = action.validate(setup.state);

    EXPECT_EQ(validation, game::MoveValidation::Valid);
}

TEST(TerrainMovementTest, ValidateReturnImpassableForWater) {
    TerrainMoveSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(1, 1, reg, setup.factionA));
    setup.state.mutableMap().tile(1, 2).setTerrainType(game::TerrainType::Water);

    game::MoveAction action(0, 1, 2);
    auto validation = action.validate(setup.state);

    EXPECT_EQ(validation, game::MoveValidation::ImpassableTerrain);
}

// ── Unit moveTo with explicit cost ───────────────────────────────────────────

TEST(TerrainMovementTest, MoveToDeductsExplicitCost) {
    auto reg = makeRegistry();
    game::Warrior unit(0, 0, reg, 1);
    EXPECT_EQ(unit.movementRemaining(), 2);

    // Deduct cost of 1.
    unit.moveTo(0, 1, 1);
    EXPECT_EQ(unit.movementRemaining(), 1);
    EXPECT_EQ(unit.row(), 0);
    EXPECT_EQ(unit.col(), 1);
}

TEST(TerrainMovementTest, MoveToDeductsHigherCost) {
    game::UnitTemplate tmpl;
    tmpl.name = "Scout";
    tmpl.maxHealth = 50;
    tmpl.attack = 5;
    tmpl.defense = 5;
    tmpl.movementRange = 5;
    tmpl.attackRange = 1;
    game::UnitTypeRegistry customReg;
    customReg.registerTemplate("Scout", tmpl);

    game::Unit unit(0, 0, customReg.getTemplate("Scout"), 1);
    EXPECT_EQ(unit.movementRemaining(), 5);

    // Deduct cost of 3 (e.g., swamp).
    unit.moveTo(0, 1, 3);
    EXPECT_EQ(unit.movementRemaining(), 2);
}

// ── Sequential moves with variable costs ─────────────────────────────────────

TEST(TerrainMovementTest, SequentialMovesWithDifferentCosts) {
    TerrainMoveSetup setup;

    game::UnitTemplate tmpl;
    tmpl.name = "Scout";
    tmpl.maxHealth = 50;
    tmpl.attack = 5;
    tmpl.defense = 5;
    tmpl.movementRange = 5;
    tmpl.attackRange = 1;
    game::UnitTypeRegistry customReg;
    customReg.registerTemplate("Scout", tmpl);

    setup.state.addUnit(std::make_unique<game::Unit>(1, 1, customReg.getTemplate("Scout"), setup.factionA));
    auto &unit = *setup.state.units()[0];

    // Move through plains (cost 1), then hills (cost 2).
    // (1,1) -> (1,2) plains, cost 1 => remaining 4
    game::MoveAction move1(0, 1, 2);
    auto r1 = move1.execute(setup.state);
    EXPECT_TRUE(r1.executed);
    EXPECT_EQ(r1.movementCostDeducted, 1);
    EXPECT_EQ(unit.movementRemaining(), 4);

    // (1,2) -> (1,3) hills, cost 2 => remaining 2
    setup.state.mutableMap().tile(1, 3).setTerrainType(game::TerrainType::Hills);
    game::MoveAction move2(0, 1, 3);
    auto r2 = move2.execute(setup.state);
    EXPECT_TRUE(r2.executed);
    EXPECT_EQ(r2.movementCostDeducted, 2);
    EXPECT_EQ(unit.movementRemaining(), 2);

    // (1,3) -> (1,4) plains, cost 1 => remaining 1
    game::MoveAction move3(0, 1, 4);
    auto r3 = move3.execute(setup.state);
    EXPECT_TRUE(r3.executed);
    EXPECT_EQ(r3.movementCostDeducted, 1);
    EXPECT_EQ(unit.movementRemaining(), 1);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
