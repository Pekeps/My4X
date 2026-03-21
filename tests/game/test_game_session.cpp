// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/GameSession.h"

#include "game/AttackAction.h"
#include "game/CaptureAction.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
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
struct SessionSetup {
    game::GameSession session;
    game::FactionId factionA = 0;
    game::FactionId factionB = 0;
    game::PlayerId playerA = 1;
    game::PlayerId playerB = 2;

    SessionSetup() : session(20, 20) {
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
// Player management tests
// ============================================================================

TEST(GameSession, AddPlayerSucceeds) {
    game::GameSession session(10, 10);
    EXPECT_TRUE(session.addPlayer(1, 1));
    EXPECT_EQ(session.playerCount(), 1);
    EXPECT_TRUE(session.hasPlayer(1));
}

TEST(GameSession, AddDuplicatePlayerFails) {
    game::GameSession session(10, 10);
    EXPECT_TRUE(session.addPlayer(1, 1));
    EXPECT_FALSE(session.addPlayer(1, 2));
    EXPECT_EQ(session.playerCount(), 1);
}

TEST(GameSession, RemovePlayerSucceeds) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    EXPECT_TRUE(session.removePlayer(1));
    EXPECT_EQ(session.playerCount(), 0);
    EXPECT_FALSE(session.hasPlayer(1));
}

TEST(GameSession, RemoveNonexistentPlayerFails) {
    game::GameSession session(10, 10);
    EXPECT_FALSE(session.removePlayer(42));
}

TEST(GameSession, GetPlayerFaction) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 5);
    EXPECT_EQ(session.getPlayerFaction(1), 5);
    EXPECT_EQ(session.getPlayerFaction(99), 0); // nonexistent
}

TEST(GameSession, GetPlayerInfo) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 3);

    const auto *info = session.getPlayerInfo(1);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->playerId, 1);
    EXPECT_EQ(info->factionId, 3);
    EXPECT_EQ(info->status, game::PlayerStatus::Connected);
    EXPECT_FALSE(info->endedTurn);

    EXPECT_EQ(session.getPlayerInfo(99), nullptr);
}

TEST(GameSession, ConnectDisconnectPlayer) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);

    EXPECT_TRUE(session.disconnectPlayer(1));
    EXPECT_EQ(session.getPlayerInfo(1)->status, game::PlayerStatus::Disconnected);

    EXPECT_TRUE(session.connectPlayer(1));
    EXPECT_EQ(session.getPlayerInfo(1)->status, game::PlayerStatus::Connected);

    // Nonexistent players.
    EXPECT_FALSE(session.connectPlayer(99));
    EXPECT_FALSE(session.disconnectPlayer(99));
}

// ============================================================================
// Turn management tests
// ============================================================================

TEST(GameSession, InitialTurnIsOne) {
    game::GameSession session(10, 10);
    EXPECT_EQ(session.currentTurn(), 1);
}

TEST(GameSession, EndTurnSinglePlayer) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);

    EXPECT_FALSE(session.allPlayersEndedTurn());
    EXPECT_TRUE(session.endTurn(1));
    EXPECT_TRUE(session.allPlayersEndedTurn());
}

TEST(GameSession, EndTurnNonexistentPlayerFails) {
    game::GameSession session(10, 10);
    EXPECT_FALSE(session.endTurn(99));
}

TEST(GameSession, AllPlayersEndedTurnRequiresAllConnected) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    session.addPlayer(2, 2);

    session.endTurn(1);
    EXPECT_FALSE(session.allPlayersEndedTurn());

    session.endTurn(2);
    EXPECT_TRUE(session.allPlayersEndedTurn());
}

TEST(GameSession, DisconnectedPlayersSkippedForEndTurn) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    session.addPlayer(2, 2);

    session.disconnectPlayer(2);
    session.endTurn(1);
    EXPECT_TRUE(session.allPlayersEndedTurn());
}

TEST(GameSession, AllPlayersEndedTurnEmptySession) {
    game::GameSession session(10, 10);
    EXPECT_FALSE(session.allPlayersEndedTurn());
}

TEST(GameSession, TryAdvanceTurnSucceeds) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);

    EXPECT_EQ(session.currentTurn(), 1);
    session.endTurn(1);
    EXPECT_TRUE(session.tryAdvanceTurn());
    EXPECT_EQ(session.currentTurn(), 2);
}

TEST(GameSession, TryAdvanceTurnFailsIfNotAllReady) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    session.addPlayer(2, 2);

    session.endTurn(1);
    EXPECT_FALSE(session.tryAdvanceTurn());
    EXPECT_EQ(session.currentTurn(), 1);
}

TEST(GameSession, TurnFlagsResetAfterAdvance) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);

    session.endTurn(1);
    EXPECT_TRUE(session.tryAdvanceTurn());
    EXPECT_EQ(session.currentTurn(), 2);

    // Flags should be reset — not ready yet.
    EXPECT_FALSE(session.allPlayersEndedTurn());

    // End turn again and advance.
    session.endTurn(1);
    EXPECT_TRUE(session.tryAdvanceTurn());
    EXPECT_EQ(session.currentTurn(), 3);
}

TEST(GameSession, MultipleTurnAdvances) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    session.addPlayer(2, 2);

    for (int turn = 1; turn <= 5; ++turn) {
        EXPECT_EQ(session.currentTurn(), turn);
        session.endTurn(1);
        session.endTurn(2);
        EXPECT_TRUE(session.tryAdvanceTurn());
    }
    EXPECT_EQ(session.currentTurn(), 6);
}

// ============================================================================
// Action processing tests
// ============================================================================

TEST(GameSession, ApplyActionInvalidPlayer) {
    game::GameSession session(10, 10);
    game::MoveAction move(0, 1, 0);
    EXPECT_EQ(session.applyAction(99, move), game::ActionResult::InvalidPlayer);
}

TEST(GameSession, ApplyMoveActionSucceeds) {
    SessionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Add a warrior for player A.
    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Move to adjacent tile (2,2) -> (2,3).
    game::MoveAction move(0, 2, 3);
    auto result = setup.session.applyAction(setup.playerA, move);
    EXPECT_EQ(result, game::ActionResult::Success);

    // Verify unit moved.
    EXPECT_EQ(state.units()[0]->row(), 2);
    EXPECT_EQ(state.units()[0]->col(), 3);
}

TEST(GameSession, ApplyMoveActionFails) {
    SessionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Add a warrior with no movement remaining.
    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    warrior->setMovementRemaining(0);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 2, 3);
    auto result = setup.session.applyAction(setup.playerA, move);
    EXPECT_EQ(result, game::ActionResult::ActionFailed);
}

TEST(GameSession, ApplyAttackActionSucceeds) {
    SessionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Add adjacent warriors from different factions at war.
    auto attackerUnit = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    auto targetUnit = std::make_unique<game::Warrior>(2, 3, unitReg, setup.factionB);
    state.addUnit(std::move(attackerUnit));
    state.addUnit(std::move(targetUnit));

    game::AttackAction attack(0, 1);
    auto result = setup.session.applyAction(setup.playerA, attack);
    EXPECT_EQ(result, game::ActionResult::Success);
}

TEST(GameSession, ApplyAttackActionFailsSameFaction) {
    SessionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Two units from the same faction.
    auto unit1 = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    auto unit2 = std::make_unique<game::Warrior>(2, 3, unitReg, setup.factionA);
    state.addUnit(std::move(unit1));
    state.addUnit(std::move(unit2));

    game::AttackAction attack(0, 1);
    auto result = setup.session.applyAction(setup.playerA, attack);
    EXPECT_EQ(result, game::ActionResult::ActionFailed);
}

// ============================================================================
// State snapshot tests
// ============================================================================

TEST(GameSession, GetStateSnapshotReturnsCurrentState) {
    game::GameSession session(15, 15);
    const auto &snapshot = session.getStateSnapshot();
    EXPECT_EQ(snapshot.map().height(), 15);
    EXPECT_EQ(snapshot.map().width(), 15);
    EXPECT_EQ(snapshot.getTurn(), 1);
}

TEST(GameSession, StateSnapshotReflectsChanges) {
    SessionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(3, 3, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    const auto &snapshot = setup.session.getStateSnapshot();
    EXPECT_EQ(snapshot.units().size(), 1);
    EXPECT_EQ(snapshot.units()[0]->row(), 3);
    EXPECT_EQ(snapshot.units()[0]->col(), 3);
}

// ============================================================================
// Session lifecycle tests
// ============================================================================

TEST(GameSession, FullSessionLifecycle) {
    // Create session.
    game::GameSession session(20, 20);
    auto &state = session.mutableState();

    // Register factions.
    auto &factionReg = state.mutableFactionRegistry();
    auto fA = factionReg.addFaction("Humans", game::FactionType::Player, 0);
    auto fB = factionReg.addFaction("Orcs", game::FactionType::Player, 1);
    state.mutableDiplomacy().setRelation(fA, fB, game::DiplomacyState::War);

    // Add players.
    game::PlayerId pA = 10;
    game::PlayerId pB = 20;
    EXPECT_TRUE(session.addPlayer(pA, fA));
    EXPECT_TRUE(session.addPlayer(pB, fB));
    EXPECT_EQ(session.playerCount(), 2);

    // Set up terrain.
    for (int r = 0; r < state.map().height(); ++r) {
        for (int c = 0; c < state.map().width(); ++c) {
            state.mutableMap().tile(r, c).setTerrainType(game::TerrainType::Plains);
        }
    }

    // Add units.
    auto unitReg = makeRegistry();
    auto wA = std::make_unique<game::Warrior>(5, 5, unitReg, fA);
    auto wB = std::make_unique<game::Warrior>(10, 10, unitReg, fB);
    state.addUnit(std::move(wA));
    state.addUnit(std::move(wB));

    // Turn 1: both players move, then end turn.
    game::MoveAction moveA(0, 5, 6);
    EXPECT_EQ(session.applyAction(pA, moveA), game::ActionResult::Success);

    game::MoveAction moveB(1, 10, 11);
    EXPECT_EQ(session.applyAction(pB, moveB), game::ActionResult::Success);

    session.endTurn(pA);
    session.endTurn(pB);
    EXPECT_TRUE(session.tryAdvanceTurn());
    EXPECT_EQ(session.currentTurn(), 2);

    // Verify units are at new positions.
    EXPECT_EQ(state.units()[0]->row(), 5);
    EXPECT_EQ(state.units()[0]->col(), 6);
    EXPECT_EQ(state.units()[1]->row(), 10);
    EXPECT_EQ(state.units()[1]->col(), 11);
}

TEST(GameSession, DisconnectedPlayerDoesNotBlockTurn) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    session.addPlayer(2, 2);
    session.addPlayer(3, 3);

    // Disconnect player 2.
    session.disconnectPlayer(2);

    // Only connected players need to end turn.
    session.endTurn(1);
    session.endTurn(3);
    EXPECT_TRUE(session.tryAdvanceTurn());
    EXPECT_EQ(session.currentTurn(), 2);
}

TEST(GameSession, ReconnectedPlayerMustEndTurn) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    session.addPlayer(2, 2);

    // Disconnect and reconnect player 2.
    session.disconnectPlayer(2);
    session.connectPlayer(2);

    // Now player 2 is connected and must end turn.
    session.endTurn(1);
    EXPECT_FALSE(session.allPlayersEndedTurn());

    session.endTurn(2);
    EXPECT_TRUE(session.allPlayersEndedTurn());
}

} // namespace
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
