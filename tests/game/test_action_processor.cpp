// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/ActionProcessor.h"

#include "game/AttackAction.h"
#include "game/Building.h"
#include "game/CaptureAction.h"
#include "game/City.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/GameSession.h"
#include "game/GameState.h"
#include "game/MoveAction.h"
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

/// Helper to set up a session with two player factions at war.
struct ProcessorSetup {
    game::GameSession session;
    game::ActionProcessor processor;
    game::FactionId factionA = 0;
    game::FactionId factionB = 0;
    game::PlayerId playerA = 1;
    game::PlayerId playerB = 2;

    ProcessorSetup() : session(20, 20) {
        auto &state = session.mutableState();
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::Player, 1);

        // Set factions at war.
        state.mutableDiplomacy().setRelation(factionA, factionB, game::DiplomacyState::War);

        // Register players.
        session.addPlayer(playerA, factionA);
        session.addPlayer(playerB, factionB);

        // Set all tiles to Plains for deterministic tests.
        for (int r = 0; r < state.map().height(); ++r) {
            for (int c = 0; c < state.map().width(); ++c) {
                state.mutableMap().tile(r, c).setTerrainType(game::TerrainType::Plains);
            }
        }
    }
};

// ============================================================================
// Player validation tests
// ============================================================================

TEST(ActionProcessor, RejectsUnknownPlayer) {
    ProcessorSetup setup;
    game::MoveAction move(0, 1, 0);
    auto result = setup.processor.processAction(setup.session, 99, move);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_TRUE(result.events.empty());
}

TEST(ActionProcessor, RejectsDisconnectedPlayer) {
    ProcessorSetup setup;
    setup.session.disconnectPlayer(setup.playerA);

    game::MoveAction move(0, 1, 0);
    auto result = setup.processor.processAction(setup.session, setup.playerA, move);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "Player is not connected");
}

// ============================================================================
// Move action tests
// ============================================================================

TEST(ActionProcessor, MoveActionSucceeds) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 5, 6);
    auto result = setup.processor.processAction(setup.session, setup.playerA, move);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.errorMessage.empty());
    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::UnitMoved);
    EXPECT_EQ(result.events[0].playerId, setup.playerA);

    // Verify unit actually moved.
    EXPECT_EQ(state.units()[0]->row(), 5);
    EXPECT_EQ(state.units()[0]->col(), 6);
}

TEST(ActionProcessor, MoveActionFailsNoMovement) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    warrior->setMovementRemaining(0);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 5, 6);
    auto result = setup.processor.processAction(setup.session, setup.playerA, move);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "No movement remaining");
    EXPECT_TRUE(result.events.empty());
}

TEST(ActionProcessor, MoveActionFailsNotAdjacent) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Try to move to a non-adjacent tile.
    game::MoveAction move(0, 10, 10);
    auto result = setup.processor.processAction(setup.session, setup.playerA, move);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ActionProcessor, MoveActionFailsImpassableTerrain) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Place a mountain at destination.
    state.mutableMap().tile(5, 6).setTerrainType(game::TerrainType::Mountain);

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 5, 6);
    auto result = setup.processor.processAction(setup.session, setup.playerA, move);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "Terrain is impassable");
}

// ============================================================================
// Attack action tests
// ============================================================================

TEST(ActionProcessor, AttackActionSucceeds) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto attacker = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    auto target = std::make_unique<game::Warrior>(5, 6, unitReg, setup.factionB);
    state.addUnit(std::move(attacker));
    state.addUnit(std::move(target));

    game::AttackAction attack(0, 1);
    auto result = setup.processor.processAction(setup.session, setup.playerA, attack);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.errorMessage.empty());

    // Should have at least the attack event.
    ASSERT_GE(result.events.size(), 1);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::UnitAttacked);
    EXPECT_EQ(result.events[0].playerId, setup.playerA);
}

TEST(ActionProcessor, AttackActionGeneratesKillEvent) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto attacker = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    auto target = std::make_unique<game::Warrior>(5, 6, unitReg, setup.factionB);
    // Reduce target HP to 1 so it dies.
    target->setHealth(1);
    state.addUnit(std::move(attacker));
    state.addUnit(std::move(target));

    game::AttackAction attack(0, 1);
    auto result = setup.processor.processAction(setup.session, setup.playerA, attack);
    EXPECT_TRUE(result.success);

    // Should have attack + kill events.
    ASSERT_GE(result.events.size(), 2);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::UnitAttacked);
    EXPECT_EQ(result.events[1].type, game::ActionEventType::UnitKilled);
    EXPECT_EQ(result.events[1].detail, "Defender killed");
}

TEST(ActionProcessor, AttackActionFailsSameFaction) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto unit1 = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    auto unit2 = std::make_unique<game::Warrior>(5, 6, unitReg, setup.factionA);
    state.addUnit(std::move(unit1));
    state.addUnit(std::move(unit2));

    game::AttackAction attack(0, 1);
    auto result = setup.processor.processAction(setup.session, setup.playerA, attack);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "Cannot attack own faction");
}

TEST(ActionProcessor, AttackActionFailsOutOfRange) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto attacker = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    auto target = std::make_unique<game::Warrior>(10, 10, unitReg, setup.factionB);
    state.addUnit(std::move(attacker));
    state.addUnit(std::move(target));

    game::AttackAction attack(0, 1);
    auto result = setup.processor.processAction(setup.session, setup.playerA, attack);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "Target is out of range");
}

// ============================================================================
// Capture action tests
// ============================================================================

TEST(ActionProcessor, CaptureActionSucceeds) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Create a city owned by faction B.
    game::City city("EnemyCity", 5, 5, static_cast<int>(setup.factionB));
    city.addTile(5, 5);
    state.addCity(std::move(city));

    // Place a warrior from faction A on the city center.
    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::CaptureAction capture(0);
    auto result = setup.processor.processAction(setup.session, setup.playerA, capture);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.errorMessage.empty());

    ASSERT_GE(result.events.size(), 1);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::CityCaptured);
    EXPECT_EQ(result.events[0].detail, "EnemyCity");
    EXPECT_EQ(result.events[0].factionId, setup.factionB);

    // Verify ownership transferred.
    EXPECT_EQ(state.cities()[0].factionId(), static_cast<int>(setup.factionA));
}

TEST(ActionProcessor, CaptureActionGeneratesEliminationEvent) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Faction B has exactly one city, no units.
    game::City city("LastCity", 5, 5, static_cast<int>(setup.factionB));
    city.addTile(5, 5);
    state.addCity(std::move(city));

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::CaptureAction capture(0);
    auto result = setup.processor.processAction(setup.session, setup.playerA, capture);
    EXPECT_TRUE(result.success);

    // Should have capture + elimination events.
    ASSERT_GE(result.events.size(), 2);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::CityCaptured);
    EXPECT_EQ(result.events[1].type, game::ActionEventType::FactionEliminated);
    EXPECT_EQ(result.events[1].factionId, setup.factionB);
}

TEST(ActionProcessor, CaptureActionFailsNoCityAtPosition) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Warrior not on any city center.
    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::CaptureAction capture(0);
    auto result = setup.processor.processAction(setup.session, setup.playerA, capture);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "No city at unit position");
}

TEST(ActionProcessor, CaptureActionFailsOwnCity) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // City owned by the capturing player's faction.
    game::City city("OwnCity", 5, 5, static_cast<int>(setup.factionA));
    city.addTile(5, 5);
    state.addCity(std::move(city));

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::CaptureAction capture(0);
    auto result = setup.processor.processAction(setup.session, setup.playerA, capture);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "City belongs to same faction");
}

// ============================================================================
// EndTurn action tests
// ============================================================================

TEST(ActionProcessor, EndTurnActionSucceeds) {
    ProcessorSetup setup;

    game::EndTurnAction endTurn;
    auto result = setup.processor.processAction(setup.session, setup.playerA, endTurn);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.errorMessage.empty());

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::TurnEnded);
    EXPECT_EQ(result.events[0].playerId, setup.playerA);

    // Verify the player's endedTurn flag is set.
    const auto *info = setup.session.getPlayerInfo(setup.playerA);
    ASSERT_NE(info, nullptr);
    EXPECT_TRUE(info->endedTurn);
}

TEST(ActionProcessor, EndTurnAdvancesTurnWhenAllReady) {
    ProcessorSetup setup;
    EXPECT_EQ(setup.session.currentTurn(), 1);

    // Player A ends turn.
    game::EndTurnAction endTurn;
    setup.processor.processAction(setup.session, setup.playerA, endTurn);
    EXPECT_EQ(setup.session.currentTurn(), 1); // Not yet.

    // Player B ends turn — both ready, should advance.
    setup.processor.processAction(setup.session, setup.playerB, endTurn);
    EXPECT_EQ(setup.session.currentTurn(), 2);
}

TEST(ActionProcessor, EndTurnDoesNotAdvanceIfNotAllReady) {
    ProcessorSetup setup;

    game::EndTurnAction endTurn;
    setup.processor.processAction(setup.session, setup.playerA, endTurn);
    EXPECT_EQ(setup.session.currentTurn(), 1);
}

// ============================================================================
// ProduceUnit action tests
// ============================================================================

TEST(ActionProcessor, ProduceUnitSucceeds) {
    ProcessorSetup setup;
    auto &state = setup.session.mutableState();

    // Create a city owned by faction A.
    game::City city("CapitalA", 5, 5, static_cast<int>(setup.factionA));
    city.addTile(5, 5);
    auto cityId = state.addCity(std::move(city));

    game::ProduceUnitAction produce;
    produce.cityId = cityId;
    produce.templateKey = "Warrior";
    produce.productionCost = 30;

    auto result = setup.processor.processAction(setup.session, setup.playerA, produce);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.errorMessage.empty());

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::ProductionQueued);
    EXPECT_EQ(result.events[0].detail, "Warrior");

    // Verify the build queue has the item.
    auto *cityPtr = state.findCity(cityId);
    ASSERT_NE(cityPtr, nullptr);
    EXPECT_FALSE(cityPtr->buildQueue().isEmpty());
    auto currentItem = cityPtr->buildQueue().currentItem();
    ASSERT_TRUE(currentItem.has_value());
    EXPECT_EQ(currentItem->templateKey, "Warrior");
}

TEST(ActionProcessor, ProduceUnitFailsWrongFaction) {
    ProcessorSetup setup;
    auto &state = setup.session.mutableState();

    // Create a city owned by faction B.
    game::City city("CapitalB", 5, 5, static_cast<int>(setup.factionB));
    city.addTile(5, 5);
    auto cityId = state.addCity(std::move(city));

    game::ProduceUnitAction produce;
    produce.cityId = cityId;
    produce.templateKey = "Warrior";
    produce.productionCost = 30;

    // Player A tries to produce in faction B's city.
    auto result = setup.processor.processAction(setup.session, setup.playerA, produce);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "City does not belong to player's faction");
}

TEST(ActionProcessor, ProduceUnitFailsCityNotFound) {
    ProcessorSetup setup;

    game::ProduceUnitAction produce;
    produce.cityId = 999;
    produce.templateKey = "Warrior";
    produce.productionCost = 30;

    auto result = setup.processor.processAction(setup.session, setup.playerA, produce);
    EXPECT_FALSE(result.success);
}

// ============================================================================
// Build action tests
// ============================================================================

TEST(ActionProcessor, BuildActionSucceeds) {
    ProcessorSetup setup;
    auto &state = setup.session.mutableState();

    game::City city("CapitalA", 5, 5, static_cast<int>(setup.factionA));
    city.addTile(5, 5);
    city.addTile(5, 6);
    auto cityId = state.addCity(std::move(city));

    game::BuildAction build;
    build.cityId = cityId;
    build.buildingType = "Farm";
    build.targetRow = 5;
    build.targetCol = 6;

    auto result = setup.processor.processAction(setup.session, setup.playerA, build);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.errorMessage.empty());

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::BuildQueued);
    EXPECT_EQ(result.events[0].detail, "Farm");
    EXPECT_EQ(result.events[0].row, 5);
    EXPECT_EQ(result.events[0].col, 6);

    // Verify the build queue has the item.
    auto *cityPtr = state.findCity(cityId);
    ASSERT_NE(cityPtr, nullptr);
    EXPECT_FALSE(cityPtr->buildQueue().isEmpty());
}

TEST(ActionProcessor, BuildActionFailsWrongFaction) {
    ProcessorSetup setup;
    auto &state = setup.session.mutableState();

    game::City city("CapitalB", 5, 5, static_cast<int>(setup.factionB));
    city.addTile(5, 5);
    auto cityId = state.addCity(std::move(city));

    game::BuildAction build;
    build.cityId = cityId;
    build.buildingType = "Farm";
    build.targetRow = 5;
    build.targetCol = 5;

    auto result = setup.processor.processAction(setup.session, setup.playerA, build);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "City does not belong to player's faction");
}

TEST(ActionProcessor, BuildActionFailsUnknownType) {
    ProcessorSetup setup;
    auto &state = setup.session.mutableState();

    game::City city("CapitalA", 5, 5, static_cast<int>(setup.factionA));
    city.addTile(5, 5);
    auto cityId = state.addCity(std::move(city));

    game::BuildAction build;
    build.cityId = cityId;
    build.buildingType = "NonexistentBuilding";
    build.targetRow = 5;
    build.targetCol = 5;

    auto result = setup.processor.processAction(setup.session, setup.playerA, build);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "Unknown building type: NonexistentBuilding");
}

TEST(ActionProcessor, BuildActionAllBuildingTypes) {
    ProcessorSetup setup;
    auto &state = setup.session.mutableState();

    game::City city("CapitalA", 5, 5, static_cast<int>(setup.factionA));
    city.addTile(5, 5);
    auto cityId = state.addCity(std::move(city));

    // Test each valid building type.
    const char *buildingTypes[] = {"Farm", "Mine", "LumberMill", "Barracks", "Market"};
    for (const auto *typeName : buildingTypes) {
        game::BuildAction build;
        build.cityId = cityId;
        build.buildingType = typeName;
        build.targetRow = 5;
        build.targetCol = 5;

        auto result = setup.processor.processAction(setup.session, setup.playerA, build);
        EXPECT_TRUE(result.success) << "Failed for building type: " << typeName;
    }
}

// ============================================================================
// ActionVariant dispatch tests
// ============================================================================

TEST(ActionProcessor, VariantDispatchesMoveCorrectly) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::ActionVariant action = game::MoveAction(0, 5, 6);
    auto result = setup.processor.processAction(setup.session, setup.playerA, action);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::UnitMoved);
}

TEST(ActionProcessor, VariantDispatchesAttackCorrectly) {
    ProcessorSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto attacker = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    auto target = std::make_unique<game::Warrior>(5, 6, unitReg, setup.factionB);
    state.addUnit(std::move(attacker));
    state.addUnit(std::move(target));

    game::ActionVariant action = game::AttackAction(0, 1);
    auto result = setup.processor.processAction(setup.session, setup.playerA, action);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::UnitAttacked);
}

TEST(ActionProcessor, VariantDispatchesEndTurnCorrectly) {
    ProcessorSetup setup;

    game::ActionVariant action = game::EndTurnAction{};
    auto result = setup.processor.processAction(setup.session, setup.playerA, action);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.events[0].type, game::ActionEventType::TurnEnded);
}

// ============================================================================
// Result structure tests
// ============================================================================

TEST(ActionProcessor, SuccessResultHasEmptyError) {
    ProcessorSetup setup;

    game::EndTurnAction endTurn;
    auto result = setup.processor.processAction(setup.session, setup.playerA, endTurn);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.errorMessage.empty());
    EXPECT_FALSE(result.events.empty());
}

TEST(ActionProcessor, FailureResultHasNoEvents) {
    ProcessorSetup setup;

    game::MoveAction move(0, 1, 0);
    auto result = setup.processor.processAction(setup.session, 99, move);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_TRUE(result.events.empty());
}

} // namespace
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
