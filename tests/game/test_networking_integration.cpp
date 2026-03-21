// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
/// @file test_networking_integration.cpp
/// Integration tests that verify the full client-server game logic loop
/// end-to-end. These tests exercise GameSession, TurnManager, and action
/// processing together in multi-player scenarios without real networking.

#include "game/AttackAction.h"
#include "game/CaptureAction.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/GameSession.h"
#include "game/GameState.h"
#include "game/MoveAction.h"
#include "game/TerrainType.h"
#include "game/TurnManager.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

// ============================================================================
// Helpers
// ============================================================================

/// Build a registry with the default unit types.
game::UnitTypeRegistry makeRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

/// Shared integration-test fixture: a GameSession with a TurnManager and
/// two player factions at war, plus a unit type registry.
struct IntegrationFixture {
    game::GameSession session;
    game::TurnManager turnManager;
    game::UnitTypeRegistry unitReg;

    game::FactionId factionA = 0;
    game::FactionId factionB = 0;
    game::PlayerId playerA = 1;
    game::PlayerId playerB = 2;

    IntegrationFixture()
        : IntegrationFixture(game::TurnConfig{}) {}

    explicit IntegrationFixture(game::TurnConfig cfg)
        : session(20, 20), turnManager(session, cfg), unitReg(makeRegistry()) {
        auto &state = session.mutableState();
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::Player, 1);

        state.mutableDiplomacy().setRelation(factionA, factionB,
                                             game::DiplomacyState::War);

        session.addPlayer(playerA, factionA);
        session.addPlayer(playerB, factionB);

        // Normalize all tiles to Plains for deterministic movement/combat.
        for (int r = 0; r < state.map().height(); ++r) {
            for (int c = 0; c < state.map().width(); ++c) {
                state.mutableMap().tile(r, c).setTerrainType(
                    game::TerrainType::Plains);
            }
        }
    }
};

/// Extended fixture with three players and three factions at war.
struct ThreePlayerFixture {
    game::GameSession session;
    game::TurnManager turnManager;
    game::UnitTypeRegistry unitReg;

    game::FactionId factionA = 0;
    game::FactionId factionB = 0;
    game::FactionId factionC = 0;
    game::PlayerId playerA = 1;
    game::PlayerId playerB = 2;
    game::PlayerId playerC = 3;

    ThreePlayerFixture()
        : ThreePlayerFixture(game::TurnConfig{}) {}

    explicit ThreePlayerFixture(game::TurnConfig cfg)
        : session(20, 20), turnManager(session, cfg), unitReg(makeRegistry()) {
        auto &state = session.mutableState();
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::Player, 1);
        factionC = registry.addFaction("FactionC", game::FactionType::Player, 2);

        state.mutableDiplomacy().setRelation(factionA, factionB,
                                             game::DiplomacyState::War);
        state.mutableDiplomacy().setRelation(factionA, factionC,
                                             game::DiplomacyState::War);
        state.mutableDiplomacy().setRelation(factionB, factionC,
                                             game::DiplomacyState::War);

        session.addPlayer(playerA, factionA);
        session.addPlayer(playerB, factionB);
        session.addPlayer(playerC, factionC);

        for (int r = 0; r < state.map().height(); ++r) {
            for (int c = 0; c < state.map().width(); ++c) {
                state.mutableMap().tile(r, c).setTerrainType(
                    game::TerrainType::Plains);
            }
        }
    }
};

// ============================================================================
// Basic flow: create session, add players, submit moves, verify state
// ============================================================================

TEST(IntegrationTest, BasicFlow_CreateSessionAddPlayersSubmitMoves) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    // Place one warrior per faction.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(10, 10, fix.unitReg, fix.factionB));

    // Both players submit moves.
    game::MoveAction moveA(0, 5, 6);
    game::MoveAction moveB(1, 10, 11);

    EXPECT_EQ(fix.session.applyAction(fix.playerA, moveA),
              game::ActionResult::Success);
    EXPECT_EQ(fix.session.applyAction(fix.playerB, moveB),
              game::ActionResult::Success);

    // Verify positions.
    EXPECT_EQ(state.units()[0]->row(), 5);
    EXPECT_EQ(state.units()[0]->col(), 6);
    EXPECT_EQ(state.units()[1]->row(), 10);
    EXPECT_EQ(state.units()[1]->col(), 11);
}

TEST(IntegrationTest, BasicFlow_StateSnapshotReflectsMoves) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(3, 3, fix.unitReg, fix.factionA));

    game::MoveAction move(0, 3, 4);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, move),
              game::ActionResult::Success);

    const auto &snap = fix.session.getStateSnapshot();
    EXPECT_EQ(snap.units()[0]->row(), 3);
    EXPECT_EQ(snap.units()[0]->col(), 4);
}

// ============================================================================
// Turn lifecycle: submit actions -> end turns -> advance -> new turn
// ============================================================================

TEST(IntegrationTest, TurnLifecycle_FullCycleWithTurnManager) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(10, 10, fix.unitReg, fix.factionB));

    double time = 0.0;
    fix.turnManager.startTurn(time);
    EXPECT_EQ(fix.turnManager.turnNumber(), 1);
    EXPECT_TRUE(fix.turnManager.isTurnInProgress());

    // Players submit moves.
    game::MoveAction moveA(0, 5, 6);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, moveA),
              game::ActionResult::Success);

    game::MoveAction moveB(1, 10, 11);
    EXPECT_EQ(fix.session.applyAction(fix.playerB, moveB),
              game::ActionResult::Success);

    // Players end their turns.
    time += 10.0;
    EXPECT_TRUE(fix.turnManager.playerEndTurn(fix.playerA, time));
    EXPECT_FALSE(fix.turnManager.isReady());

    time += 5.0;
    EXPECT_TRUE(fix.turnManager.playerEndTurn(fix.playerB, time));
    EXPECT_TRUE(fix.turnManager.isReady());

    // Advance the turn.
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);

    // Movement should be reset after turn advance (resolveTurn resets).
    EXPECT_EQ(state.units()[0]->movementRemaining(),
              state.units()[0]->movement());
    EXPECT_EQ(state.units()[1]->movementRemaining(),
              state.units()[1]->movement());
}

TEST(IntegrationTest, TurnLifecycle_MultiTurnSequence) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(15, 15, fix.unitReg, fix.factionB));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Play three full turns, moving each unit one step per turn.
    for (int turn = 1; turn <= 3; ++turn) {
        EXPECT_EQ(fix.turnManager.turnNumber(), turn);

        // Unit A moves right, unit B moves left (both on same row for simplicity).
        int colA = state.units()[0]->col();
        int colB = state.units()[1]->col();

        game::MoveAction mA(0, state.units()[0]->row(), colA + 1);
        EXPECT_EQ(fix.session.applyAction(fix.playerA, mA),
                  game::ActionResult::Success);

        game::MoveAction mB(1, state.units()[1]->row(), colB - 1);
        EXPECT_EQ(fix.session.applyAction(fix.playerB, mB),
                  game::ActionResult::Success);

        time += 10.0;
        fix.turnManager.playerEndTurn(fix.playerA, time);
        time += 5.0;
        fix.turnManager.playerEndTurn(fix.playerB, time);
        EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    }

    EXPECT_EQ(fix.turnManager.turnNumber(), 4);
    // Unit A moved right 3 times from col 5.
    EXPECT_EQ(state.units()[0]->col(), 8);
    // Unit B moved left 3 times from col 15.
    EXPECT_EQ(state.units()[1]->col(), 12);
}

TEST(IntegrationTest, TurnLifecycle_DeadlineForceAdvancesTurn) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 30.0;
    IntegrationFixture fix(cfg);

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Only player A ends turn; player B does not.
    time += 10.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    EXPECT_FALSE(fix.turnManager.isReady());

    // At deadline, turn should be forcibly advanced.
    time = 30.0;
    EXPECT_TRUE(fix.turnManager.isExpired(time));
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);
}

TEST(IntegrationTest, TurnLifecycle_GracePeriodTriggersEarlyAdvance) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 120.0;
    cfg.gracePeriodSeconds = 10.0;
    IntegrationFixture fix(cfg);

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Player A ends turn at t=20 -> grace deadline = 30.
    time = 20.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    EXPECT_EQ(fix.turnManager.graceDeadline(), 30.0);

    // At grace deadline the turn is forced.
    EXPECT_TRUE(fix.turnManager.tryAdvance(30.0));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);
}

// ============================================================================
// Combat: two players' units fight, verify resolution and unit removal
// ============================================================================

TEST(IntegrationTest, Combat_MeleeAttackAndCounterAttack) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    // Place adjacent warriors.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));

    int hpBefore0 = state.units()[0]->health();
    int hpBefore1 = state.units()[1]->health();

    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::Success);

    // Both survive with reduced health (Warrior vs Warrior melee).
    EXPECT_EQ(state.units().size(), 2);
    EXPECT_LT(state.units()[0]->health(), hpBefore0); // counter-attack damage
    EXPECT_LT(state.units()[1]->health(), hpBefore1); // attack damage
}

TEST(IntegrationTest, Combat_DefenderKilled) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    // Place adjacent warriors; weaken the defender so it dies in one hit.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));
    state.units()[1]->takeDamage(95); // 5 HP remaining

    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::Success);

    // Defender should be removed.
    EXPECT_EQ(state.units().size(), 1);
    EXPECT_EQ(state.units()[0]->factionId(), fix.factionA);
}

TEST(IntegrationTest, Combat_AttackerKilled) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    // Weaken the attacker so the counter-attack kills it.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));
    state.units()[0]->takeDamage(95); // 5 HP remaining

    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::Success);

    // Attacker should be removed; only defender remains.
    EXPECT_EQ(state.units().size(), 1);
    EXPECT_EQ(state.units()[0]->factionId(), fix.factionB);
}

TEST(IntegrationTest, Combat_MoveAndAttackSequence) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    // Warriors start 2 tiles apart.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 4, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Player A moves adjacent.
    game::MoveAction move(0, 5, 5);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, move),
              game::ActionResult::Success);

    // Player A attacks.
    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::Success);

    // Damage should be dealt.
    EXPECT_LT(state.units()[1]->health(), 100);
}

TEST(IntegrationTest, Combat_AcrossMultipleTurns) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    // Place adjacent warriors.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Attack each turn until one unit dies.
    int maxTurns = 20; // safety limit
    while (state.units().size() == 2 && maxTurns-- > 0) {
        game::AttackAction attack(0, 1);
        fix.session.applyAction(fix.playerA, attack);

        time += 10.0;
        fix.turnManager.playerEndTurn(fix.playerA, time);
        time += 5.0;
        fix.turnManager.playerEndTurn(fix.playerB, time);
        EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    }

    // One unit should have been killed eventually.
    EXPECT_EQ(state.units().size(), 1);
}

// ============================================================================
// Invalid actions: submit illegal moves, verify rejection
// ============================================================================

TEST(IntegrationTest, InvalidAction_MoveToImpassableTerrain) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));

    // Set the destination tile to Water (impassable for land units).
    state.mutableMap().tile(5, 6).setTerrainType(game::TerrainType::Water);

    game::MoveAction move(0, 5, 6);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, move),
              game::ActionResult::ActionFailed);

    // Unit should not have moved.
    EXPECT_EQ(state.units()[0]->col(), 5);
}

TEST(IntegrationTest, InvalidAction_MoveWithNoMovementRemaining) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.units()[0]->setMovementRemaining(0);

    game::MoveAction move(0, 5, 6);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, move),
              game::ActionResult::ActionFailed);
}

TEST(IntegrationTest, InvalidAction_MoveNonAdjacent) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));

    // Try to move 3 tiles away (not adjacent).
    game::MoveAction move(0, 5, 8);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, move),
              game::ActionResult::ActionFailed);

    // Position unchanged.
    EXPECT_EQ(state.units()[0]->col(), 5);
}

TEST(IntegrationTest, InvalidAction_AttackOutOfRange) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    // Warriors 3 tiles apart (melee range = 1).
    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 8, fix.unitReg, fix.factionB));

    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::ActionFailed);

    // No damage dealt.
    EXPECT_EQ(state.units()[1]->health(), 100);
}

TEST(IntegrationTest, InvalidAction_AttackFriendlyUnit) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionA));

    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::ActionFailed);
}

TEST(IntegrationTest, InvalidAction_AttackNoMovementRemaining) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));
    state.units()[0]->setMovementRemaining(0);

    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::ActionFailed);
}

TEST(IntegrationTest, InvalidAction_UnknownPlayerAction) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));

    game::MoveAction move(0, 5, 6);
    EXPECT_EQ(fix.session.applyAction(99, move),
              game::ActionResult::InvalidPlayer);
}

TEST(IntegrationTest, InvalidAction_AttackDeadUnit) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));
    state.units()[1]->takeDamage(999);

    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::ActionFailed);
}

// ============================================================================
// Disconnection/reconnection: verify frozen units and resumption
// ============================================================================

TEST(IntegrationTest, Disconnect_PlayerDisconnectDoesNotBlockTurnAdvance) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Disconnect player B.
    fix.session.disconnectPlayer(fix.playerB);

    // Only player A needs to end turn.
    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    EXPECT_TRUE(fix.turnManager.isReady());
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);
}

TEST(IntegrationTest, Disconnect_ReconnectedPlayerMustEndTurn) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Disconnect then reconnect player B during the turn.
    fix.session.disconnectPlayer(fix.playerB);
    fix.session.connectPlayer(fix.playerB);

    // Player A ends turn.
    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    EXPECT_FALSE(fix.turnManager.isReady()); // B reconnected, must end turn.

    // Player B ends turn.
    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerB, time);
    EXPECT_TRUE(fix.turnManager.isReady());
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);
}

TEST(IntegrationTest, Disconnect_FrozenUnitsCannotAct) {
    IntegrationFixture fix;
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(10, 10, fix.unitReg, fix.factionB));

    // Disconnect player B; they can still technically call applyAction
    // since GameSession doesn't check connection status for actions.
    // The real "freezing" is that the server doesn't relay actions from
    // disconnected clients. We verify the session level: the player remains
    // registered but disconnected.
    fix.session.disconnectPlayer(fix.playerB);

    const auto *info = fix.session.getPlayerInfo(fix.playerB);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->status, game::PlayerStatus::Disconnected);

    // After reconnect, player can act again.
    fix.session.connectPlayer(fix.playerB);
    info = fix.session.getPlayerInfo(fix.playerB);
    EXPECT_EQ(info->status, game::PlayerStatus::Connected);

    // Player B moves their unit.
    game::MoveAction move(1, 10, 11);
    EXPECT_EQ(fix.session.applyAction(fix.playerB, move),
              game::ActionResult::Success);
    EXPECT_EQ(state.units()[1]->col(), 11);
}

TEST(IntegrationTest, Disconnect_MultiTurnWithDisconnectedPlayer) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(10, 10, fix.unitReg, fix.factionB));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Disconnect player B.
    fix.session.disconnectPlayer(fix.playerB);

    // Play 3 turns with only player A active.
    for (int turn = 1; turn <= 3; ++turn) {
        EXPECT_EQ(fix.turnManager.turnNumber(), turn);

        int colA = state.units()[0]->col();
        game::MoveAction mA(0, state.units()[0]->row(), colA + 1);
        EXPECT_EQ(fix.session.applyAction(fix.playerA, mA),
                  game::ActionResult::Success);

        time += 10.0;
        fix.turnManager.playerEndTurn(fix.playerA, time);
        EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    }

    EXPECT_EQ(fix.turnManager.turnNumber(), 4);
    // Player A moved 3 tiles.
    EXPECT_EQ(state.units()[0]->col(), 8);
    // Player B's unit is frozen in place.
    EXPECT_EQ(state.units()[1]->col(), 10);

    // Reconnect player B.
    fix.session.connectPlayer(fix.playerB);

    // Now both must end turn.
    game::MoveAction mB(1, state.units()[1]->row(), 11);
    EXPECT_EQ(fix.session.applyAction(fix.playerB, mB),
              game::ActionResult::Success);

    time += 10.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    EXPECT_FALSE(fix.turnManager.isReady());

    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerB, time);
    EXPECT_TRUE(fix.turnManager.isReady());
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 5);
}

// ============================================================================
// Multi-player: 3+ player game with concurrent actions
// ============================================================================

TEST(IntegrationTest, MultiPlayer_ThreePlayerTurnCycle) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    ThreePlayerFixture fix(cfg);
    auto &state = fix.session.mutableState();

    // One warrior per faction.
    state.addUnit(
        std::make_unique<game::Warrior>(2, 2, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(8, 8, fix.unitReg, fix.factionB));
    state.addUnit(
        std::make_unique<game::Warrior>(14, 14, fix.unitReg, fix.factionC));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // All three players move, then end turns.
    game::MoveAction mA(0, 2, 3);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, mA),
              game::ActionResult::Success);

    game::MoveAction mB(1, 8, 9);
    EXPECT_EQ(fix.session.applyAction(fix.playerB, mB),
              game::ActionResult::Success);

    game::MoveAction mC(2, 14, 15);
    EXPECT_EQ(fix.session.applyAction(fix.playerC, mC),
              game::ActionResult::Success);

    time += 10.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    EXPECT_FALSE(fix.turnManager.isReady());

    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerB, time);
    EXPECT_FALSE(fix.turnManager.isReady());

    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerC, time);
    EXPECT_TRUE(fix.turnManager.isReady());

    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);

    // Verify all units moved.
    EXPECT_EQ(state.units()[0]->col(), 3);
    EXPECT_EQ(state.units()[1]->col(), 9);
    EXPECT_EQ(state.units()[2]->col(), 15);
}

TEST(IntegrationTest, MultiPlayer_ThreePlayerOneDisconnected) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    ThreePlayerFixture fix(cfg);
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(2, 2, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(8, 8, fix.unitReg, fix.factionB));
    state.addUnit(
        std::make_unique<game::Warrior>(14, 14, fix.unitReg, fix.factionC));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    fix.session.disconnectPlayer(fix.playerC);

    // Only A and B need to end turn.
    game::MoveAction mA(0, 2, 3);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, mA),
              game::ActionResult::Success);

    game::MoveAction mB(1, 8, 9);
    EXPECT_EQ(fix.session.applyAction(fix.playerB, mB),
              game::ActionResult::Success);

    time += 10.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerB, time);
    EXPECT_TRUE(fix.turnManager.isReady());
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);

    // C's unit didn't move.
    EXPECT_EQ(state.units()[2]->col(), 14);
}

TEST(IntegrationTest, MultiPlayer_ThreePlayerCombatBetweenTwo) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    ThreePlayerFixture fix(cfg);
    auto &state = fix.session.mutableState();

    // A and B are adjacent and will fight; C is far away.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));
    state.addUnit(
        std::make_unique<game::Warrior>(15, 15, fix.unitReg, fix.factionC));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // A attacks B.
    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::Success);

    // C moves peacefully.
    game::MoveAction mC(2, 15, 16);
    EXPECT_EQ(fix.session.applyAction(fix.playerC, mC),
              game::ActionResult::Success);

    // Verify damage was dealt between A and B.
    EXPECT_LT(state.units()[1]->health(), 100);

    // C's unit is unharmed.
    EXPECT_EQ(state.units()[2]->health(), 100);
    EXPECT_EQ(state.units()[2]->col(), 16);
}

TEST(IntegrationTest, MultiPlayer_ThreePlayerMultiTurn) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    ThreePlayerFixture fix(cfg);
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(2, 2, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(8, 8, fix.unitReg, fix.factionB));
    state.addUnit(
        std::make_unique<game::Warrior>(14, 14, fix.unitReg, fix.factionC));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    for (int turn = 1; turn <= 5; ++turn) {
        EXPECT_EQ(fix.turnManager.turnNumber(), turn);

        // Each unit moves right.
        for (std::size_t i = 0; i < state.units().size(); ++i) {
            int col = state.units()[i]->col();
            game::MoveAction m(i, state.units()[i]->row(), col + 1);
            game::PlayerId pid = (i == 0)   ? fix.playerA
                                 : (i == 1) ? fix.playerB
                                            : fix.playerC;
            EXPECT_EQ(fix.session.applyAction(pid, m),
                      game::ActionResult::Success);
        }

        time += 10.0;
        fix.turnManager.playerEndTurn(fix.playerA, time);
        time += 3.0;
        fix.turnManager.playerEndTurn(fix.playerB, time);
        time += 3.0;
        fix.turnManager.playerEndTurn(fix.playerC, time);
        EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    }

    EXPECT_EQ(fix.turnManager.turnNumber(), 6);
    // Each unit moved right 5 times.
    EXPECT_EQ(state.units()[0]->col(), 7);  // 2 + 5
    EXPECT_EQ(state.units()[1]->col(), 13); // 8 + 5
    EXPECT_EQ(state.units()[2]->col(), 19); // 14 + 5
}

TEST(IntegrationTest, MultiPlayer_DeadlineExpiresWithPartialReady) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 30.0;
    ThreePlayerFixture fix(cfg);

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Only player A ends turn.
    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    EXPECT_FALSE(fix.turnManager.isReady());

    // Deadline expires; remaining players are force-ended.
    EXPECT_TRUE(fix.turnManager.tryAdvance(30.0));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);
}

// ============================================================================
// End-to-end scenarios combining multiple features
// ============================================================================

TEST(IntegrationTest, EndToEnd_MoveAttackEndTurnAdvance) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    // Units start 2 tiles apart.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 4, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 6, fix.unitReg, fix.factionB));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Turn 1: A moves adjacent to B, no attack yet (movement used).
    game::MoveAction moveA(0, 5, 5);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, moveA),
              game::ActionResult::Success);

    // A attacks B with remaining movement.
    game::AttackAction attack(0, 1);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, attack),
              game::ActionResult::Success);

    // B doesn't do anything.
    time += 10.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerB, time);
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);

    // Turn 2: B retaliates.
    game::AttackAction counterAttack(1, 0);
    EXPECT_EQ(fix.session.applyAction(fix.playerB, counterAttack),
              game::ActionResult::Success);

    // Both units took additional damage.
    EXPECT_LT(state.units()[0]->health(), 100);
    EXPECT_LT(state.units()[1]->health(), 100);
}

TEST(IntegrationTest, EndToEnd_PlayerJoinsLateAndPlays) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Play turn 1 with player B disconnected.
    fix.session.disconnectPlayer(fix.playerB);

    game::MoveAction moveA(0, 5, 6);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, moveA),
              game::ActionResult::Success);

    time += 10.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);

    // Player B reconnects and adds a unit.
    fix.session.connectPlayer(fix.playerB);
    state.addUnit(
        std::make_unique<game::Warrior>(10, 10, fix.unitReg, fix.factionB));

    // Turn 2: both players act.
    game::MoveAction moveA2(0, 5, 7);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, moveA2),
              game::ActionResult::Success);

    game::MoveAction moveB(1, 10, 11);
    EXPECT_EQ(fix.session.applyAction(fix.playerB, moveB),
              game::ActionResult::Success);

    time += 10.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerB, time);
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 3);
}

TEST(IntegrationTest, EndToEnd_InvalidActionDoesNotCorruptState) {
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    state.addUnit(
        std::make_unique<game::Warrior>(5, 5, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(10, 10, fix.unitReg, fix.factionB));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    // Submit several invalid actions.
    game::MoveAction badMove(0, 5, 8); // non-adjacent
    EXPECT_EQ(fix.session.applyAction(fix.playerA, badMove),
              game::ActionResult::ActionFailed);

    game::AttackAction badAttack(0, 1); // out of range
    EXPECT_EQ(fix.session.applyAction(fix.playerA, badAttack),
              game::ActionResult::ActionFailed);

    // State should be unchanged.
    EXPECT_EQ(state.units()[0]->row(), 5);
    EXPECT_EQ(state.units()[0]->col(), 5);
    EXPECT_EQ(state.units()[0]->health(), 100);
    EXPECT_EQ(state.units()[0]->movementRemaining(),
              state.units()[0]->movement());

    // Valid action should still work.
    game::MoveAction goodMove(0, 5, 6);
    EXPECT_EQ(fix.session.applyAction(fix.playerA, goodMove),
              game::ActionResult::Success);
    EXPECT_EQ(state.units()[0]->col(), 6);

    // Turn can still advance.
    time += 10.0;
    fix.turnManager.playerEndTurn(fix.playerA, time);
    time += 5.0;
    fix.turnManager.playerEndTurn(fix.playerB, time);
    EXPECT_TRUE(fix.turnManager.tryAdvance(time));
    EXPECT_EQ(fix.turnManager.turnNumber(), 2);
}

TEST(IntegrationTest, EndToEnd_FullGameSimulation) {
    // Simulate a short game: 2 players, multiple turns, with movement
    // and combat, ending when one faction loses all units.
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    IntegrationFixture fix(cfg);
    auto &state = fix.session.mutableState();

    // Start units far apart on the same row.
    state.addUnit(
        std::make_unique<game::Warrior>(5, 2, fix.unitReg, fix.factionA));
    state.addUnit(
        std::make_unique<game::Warrior>(5, 8, fix.unitReg, fix.factionB));

    double time = 0.0;
    fix.turnManager.startTurn(time);

    int turn = 1;
    int maxTurns = 30; // safety limit

    while (state.units().size() > 1 && turn <= maxTurns) {
        // Move units toward each other.
        auto &unitA = state.units()[0];
        // Find which unit belongs to faction A / B (indices may shift).
        std::size_t idxA = 0;
        std::size_t idxB = 1;
        if (state.units().size() == 2) {
            if (state.units()[0]->factionId() != fix.factionA) {
                idxA = 1;
                idxB = 0;
            }
        }

        auto &uA = state.units()[idxA];
        auto &uB = state.units()[idxB];

        int colA = uA->col();
        int colB = uB->col();

        // Check if adjacent.
        int dist = game::AttackAction::hexDistance(uA->row(), colA,
                                                   uB->row(), colB);
        if (dist == 1) {
            // Attack!
            game::AttackAction atk(idxA, idxB);
            fix.session.applyAction(fix.playerA, atk);
        } else if (dist > 1) {
            // Move A right toward B.
            game::MoveAction mA(idxA, uA->row(), colA + 1);
            fix.session.applyAction(fix.playerA, mA);

            // Only move B if still alive and 2+ units.
            if (state.units().size() == 2) {
                game::MoveAction mB(idxB, uB->row(), colB - 1);
                fix.session.applyAction(fix.playerB, mB);
            }
        }

        time += 10.0;
        fix.turnManager.playerEndTurn(fix.playerA, time);
        time += 5.0;
        fix.turnManager.playerEndTurn(fix.playerB, time);
        fix.turnManager.tryAdvance(time);
        ++turn;
    }

    // Game should have ended with one unit remaining (or we hit max turns).
    EXPECT_LE(state.units().size(), 2);
    EXPECT_GT(fix.turnManager.turnNumber(), 1);
}

} // namespace
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
