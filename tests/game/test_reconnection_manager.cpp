// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/ReconnectionManager.h"

#include "game/GameSession.h"
#include "game/GameState.h"
#include "game/Serialization.h"
#include "game/TerrainType.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>

namespace {

using Clock = std::chrono::steady_clock;

/// Build a registry with the default unit types.
game::UnitTypeRegistry makeRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

/// Helper to set up a session with two players and a ReconnectionManager.
struct ReconnectionSetup {
    game::GameSession session;
    game::FactionId factionA = 0;
    game::FactionId factionB = 0;
    game::PlayerId playerA = 1;
    game::PlayerId playerB = 2;
    Clock::time_point baseTime = Clock::now();
    game::ReconnectionManager manager;

    ReconnectionSetup()
        : session(10, 10), manager(session) {
        auto &state = session.mutableState();
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::Player, 1);

        session.addPlayer(playerA, factionA);
        session.addPlayer(playerB, factionB);

        manager.registerPlayer(playerA, factionA);
        manager.registerPlayer(playerB, factionB);

        // Set all tiles to Plains.
        for (int r = 0; r < state.map().height(); ++r) {
            for (int c = 0; c < state.map().width(); ++c) {
                state.mutableMap().tile(r, c).setTerrainType(game::TerrainType::Plains);
            }
        }
    }
};

// ============================================================================
// Registration tests
// ============================================================================

TEST(ReconnectionManager, RegisterPlayerSucceeds) {
    game::GameSession session(10, 10);
    game::ReconnectionManager mgr(session);
    EXPECT_TRUE(mgr.registerPlayer(1, 1));
    EXPECT_EQ(mgr.trackedPlayerCount(), 1);
}

TEST(ReconnectionManager, RegisterDuplicateFails) {
    game::GameSession session(10, 10);
    game::ReconnectionManager mgr(session);
    EXPECT_TRUE(mgr.registerPlayer(1, 1));
    EXPECT_FALSE(mgr.registerPlayer(1, 2));
    EXPECT_EQ(mgr.trackedPlayerCount(), 1);
}

TEST(ReconnectionManager, UnregisterPlayerSucceeds) {
    game::GameSession session(10, 10);
    game::ReconnectionManager mgr(session);
    mgr.registerPlayer(1, 1);
    EXPECT_TRUE(mgr.unregisterPlayer(1));
    EXPECT_EQ(mgr.trackedPlayerCount(), 0);
}

TEST(ReconnectionManager, UnregisterNonexistentFails) {
    game::GameSession session(10, 10);
    game::ReconnectionManager mgr(session);
    EXPECT_FALSE(mgr.unregisterPlayer(99));
}

// ============================================================================
// Disconnect tests
// ============================================================================

TEST(ReconnectionManager, HandleDisconnectSucceeds) {
    ReconnectionSetup setup;

    EXPECT_TRUE(setup.manager.handleDisconnect(setup.playerA, setup.baseTime));

    const auto *info = setup.manager.getConnectionInfo(setup.playerA);
    ASSERT_NE(info, nullptr);
    EXPECT_FALSE(info->connected);
    EXPECT_EQ(setup.manager.disconnectedPlayerCount(), 1);
    EXPECT_EQ(setup.manager.connectedPlayerCount(), 1);

    // Verify the underlying session was updated.
    const auto *sessionInfo = setup.session.getPlayerInfo(setup.playerA);
    ASSERT_NE(sessionInfo, nullptr);
    EXPECT_EQ(sessionInfo->status, game::PlayerStatus::Disconnected);
}

TEST(ReconnectionManager, HandleDisconnectUnknownPlayerFails) {
    ReconnectionSetup setup;
    EXPECT_FALSE(setup.manager.handleDisconnect(99, setup.baseTime));
}

TEST(ReconnectionManager, HandleDisconnectAlreadyDisconnectedFails) {
    ReconnectionSetup setup;
    EXPECT_TRUE(setup.manager.handleDisconnect(setup.playerA, setup.baseTime));
    EXPECT_FALSE(setup.manager.handleDisconnect(setup.playerA, setup.baseTime));
}

// ============================================================================
// Reconnect tests
// ============================================================================

TEST(ReconnectionManager, HandleReconnectSucceeds) {
    ReconnectionSetup setup;

    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);

    auto reconnectTime = setup.baseTime + std::chrono::seconds(30);
    auto result = setup.manager.handleReconnect(setup.playerA, setup.factionA, reconnectTime);
    EXPECT_EQ(result, game::ReconnectResult::Success);

    // Player should be connected again.
    const auto *info = setup.manager.getConnectionInfo(setup.playerA);
    ASSERT_NE(info, nullptr);
    EXPECT_TRUE(info->connected);

    // Session should reflect reconnection.
    const auto *sessionInfo = setup.session.getPlayerInfo(setup.playerA);
    ASSERT_NE(sessionInfo, nullptr);
    EXPECT_EQ(sessionInfo->status, game::PlayerStatus::Connected);

    // Snapshot should be available.
    EXPECT_TRUE(setup.manager.lastSnapshot().has_value());
    EXPECT_FALSE(setup.manager.lastSnapshot()->empty());
}

TEST(ReconnectionManager, HandleReconnectUnknownPlayerFails) {
    ReconnectionSetup setup;
    auto result = setup.manager.handleReconnect(99, 1, setup.baseTime);
    EXPECT_EQ(result, game::ReconnectResult::UnknownPlayer);
}

TEST(ReconnectionManager, HandleReconnectWrongFactionFails) {
    ReconnectionSetup setup;
    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);

    auto result = setup.manager.handleReconnect(setup.playerA, setup.factionB, setup.baseTime);
    EXPECT_EQ(result, game::ReconnectResult::UnknownPlayer);
}

TEST(ReconnectionManager, HandleReconnectAlreadyConnectedFails) {
    ReconnectionSetup setup;
    auto result = setup.manager.handleReconnect(setup.playerA, setup.factionA, setup.baseTime);
    EXPECT_EQ(result, game::ReconnectResult::AlreadyConnected);
}

TEST(ReconnectionManager, HandleReconnectAfterTimeoutFails) {
    ReconnectionSetup setup;

    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);

    // Try to reconnect after the timeout has expired.
    auto reconnectTime = setup.baseTime + game::ReconnectionManager::DEFAULT_DISCONNECT_TIMEOUT;
    auto result = setup.manager.handleReconnect(setup.playerA, setup.factionA, reconnectTime);
    EXPECT_EQ(result, game::ReconnectResult::TimedOut);
    EXPECT_TRUE(setup.manager.isEliminated(setup.playerA));
}

// ============================================================================
// State snapshot tests
// ============================================================================

TEST(ReconnectionManager, SnapshotContainsCurrentState) {
    ReconnectionSetup setup;
    auto unitReg = makeRegistry();

    // Add a unit to the game state.
    auto warrior = std::make_unique<game::Warrior>(3, 3, unitReg, setup.factionA);
    setup.session.mutableState().addUnit(std::move(warrior));

    // Disconnect and reconnect.
    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);
    auto reconnectTime = setup.baseTime + std::chrono::seconds(10);
    auto result = setup.manager.handleReconnect(setup.playerA, setup.factionA, reconnectTime);
    EXPECT_EQ(result, game::ReconnectResult::Success);

    // Deserialize the snapshot and verify it matches.
    ASSERT_TRUE(setup.manager.lastSnapshot().has_value());
    auto restored = game::deserializeGameState(setup.manager.lastSnapshot().value());
    EXPECT_EQ(restored.getTurn(), setup.session.getStateSnapshot().getTurn());
    EXPECT_EQ(restored.units().size(), 1);
    EXPECT_EQ(restored.units()[0]->row(), 3);
    EXPECT_EQ(restored.units()[0]->col(), 3);
}

// ============================================================================
// Timeout / elimination tests
// ============================================================================

TEST(ReconnectionManager, CheckTimeoutsEliminatesExpiredPlayers) {
    ReconnectionSetup setup;
    auto unitReg = makeRegistry();

    // Add units for both factions.
    auto unitA = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    auto unitB = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionB);
    setup.session.mutableState().addUnit(std::move(unitA));
    setup.session.mutableState().addUnit(std::move(unitB));

    // Disconnect player A.
    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);

    // Advance time past the timeout.
    auto checkTime = setup.baseTime + game::ReconnectionManager::DEFAULT_DISCONNECT_TIMEOUT;
    auto eliminated = setup.manager.checkTimeouts(checkTime);

    EXPECT_EQ(eliminated.size(), 1);
    EXPECT_EQ(eliminated[0], setup.playerA);
    EXPECT_TRUE(setup.manager.isEliminated(setup.playerA));

    // Player A's units should be removed, player B's should remain.
    const auto &units = setup.session.getStateSnapshot().units();
    EXPECT_EQ(units.size(), 1);
    EXPECT_EQ(units[0]->factionId(), setup.factionB);
}

TEST(ReconnectionManager, CheckTimeoutsDoesNothingBeforeExpiry) {
    ReconnectionSetup setup;

    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);

    // Check just before timeout.
    auto checkTime = setup.baseTime + game::ReconnectionManager::DEFAULT_DISCONNECT_TIMEOUT - std::chrono::seconds(1);
    auto eliminated = setup.manager.checkTimeouts(checkTime);

    EXPECT_TRUE(eliminated.empty());
    EXPECT_FALSE(setup.manager.isEliminated(setup.playerA));
    EXPECT_EQ(setup.manager.trackedPlayerCount(), 2);
}

TEST(ReconnectionManager, CheckTimeoutsConnectedPlayersUnaffected) {
    ReconnectionSetup setup;

    // Don't disconnect anyone — even if time advances, nobody is eliminated.
    auto checkTime = setup.baseTime + game::ReconnectionManager::DEFAULT_DISCONNECT_TIMEOUT * 2;
    auto eliminated = setup.manager.checkTimeouts(checkTime);

    EXPECT_TRUE(eliminated.empty());
    EXPECT_EQ(setup.manager.trackedPlayerCount(), 2);
}

TEST(ReconnectionManager, EliminatedPlayerCannotReconnect) {
    ReconnectionSetup setup;

    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);

    // Force timeout.
    auto checkTime = setup.baseTime + game::ReconnectionManager::DEFAULT_DISCONNECT_TIMEOUT;
    setup.manager.checkTimeouts(checkTime);

    // Try to reconnect after elimination.
    auto result = setup.manager.handleReconnect(setup.playerA, setup.factionA, checkTime);
    EXPECT_EQ(result, game::ReconnectResult::TimedOut);
}

// ============================================================================
// Custom timeout tests
// ============================================================================

TEST(ReconnectionManager, CustomTimeoutIsRespected) {
    game::GameSession session(10, 10);
    auto &registry = session.mutableState().mutableFactionRegistry();
    auto fA = registry.addFaction("A", game::FactionType::Player, 0);
    session.addPlayer(1, fA);

    auto customTimeout = std::chrono::seconds(60);
    game::ReconnectionManager mgr(session, customTimeout);
    mgr.registerPlayer(1, fA);

    EXPECT_EQ(mgr.disconnectTimeout(), customTimeout);

    auto now = Clock::now();
    mgr.handleDisconnect(1, now);

    // Should survive at 59 seconds.
    auto before = now + std::chrono::seconds(59);
    EXPECT_TRUE(mgr.checkTimeouts(before).empty());

    // Should be eliminated at 60 seconds.
    auto at = now + customTimeout;
    auto eliminated = mgr.checkTimeouts(at);
    EXPECT_EQ(eliminated.size(), 1);
}

// ============================================================================
// Multiple disconnects / reconnects
// ============================================================================

TEST(ReconnectionManager, DisconnectReconnectMultipleTimes) {
    ReconnectionSetup setup;

    auto t = setup.baseTime;

    // First disconnect/reconnect cycle.
    EXPECT_TRUE(setup.manager.handleDisconnect(setup.playerA, t));
    t += std::chrono::seconds(10);
    EXPECT_EQ(setup.manager.handleReconnect(setup.playerA, setup.factionA, t),
              game::ReconnectResult::Success);

    // Second disconnect/reconnect cycle.
    t += std::chrono::seconds(5);
    EXPECT_TRUE(setup.manager.handleDisconnect(setup.playerA, t));
    t += std::chrono::seconds(20);
    EXPECT_EQ(setup.manager.handleReconnect(setup.playerA, setup.factionA, t),
              game::ReconnectResult::Success);

    EXPECT_TRUE(setup.manager.getConnectionInfo(setup.playerA)->connected);
    EXPECT_EQ(setup.manager.connectedPlayerCount(), 2);
}

TEST(ReconnectionManager, MultiplePlayersDisconnectIndependently) {
    ReconnectionSetup setup;

    auto t = setup.baseTime;

    // Both disconnect at different times.
    setup.manager.handleDisconnect(setup.playerA, t);
    setup.manager.handleDisconnect(setup.playerB, t + std::chrono::seconds(60));

    EXPECT_EQ(setup.manager.disconnectedPlayerCount(), 2);
    EXPECT_EQ(setup.manager.connectedPlayerCount(), 0);

    // Reconnect player A but not B.
    auto reconnectTime = t + std::chrono::seconds(30);
    EXPECT_EQ(setup.manager.handleReconnect(setup.playerA, setup.factionA, reconnectTime),
              game::ReconnectResult::Success);

    EXPECT_EQ(setup.manager.connectedPlayerCount(), 1);
    EXPECT_EQ(setup.manager.disconnectedPlayerCount(), 1);
}

// ============================================================================
// Integration: disconnected player units remain but don't act
// ============================================================================

TEST(ReconnectionManager, DisconnectedPlayerUnitsRemainOnMap) {
    ReconnectionSetup setup;
    auto unitReg = makeRegistry();

    auto warrior = std::make_unique<game::Warrior>(4, 4, unitReg, setup.factionA);
    setup.session.mutableState().addUnit(std::move(warrior));

    // Disconnect player A.
    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);

    // Unit should still exist on the map.
    const auto &units = setup.session.getStateSnapshot().units();
    EXPECT_EQ(units.size(), 1);
    EXPECT_EQ(units[0]->row(), 4);
    EXPECT_EQ(units[0]->col(), 4);
}

TEST(ReconnectionManager, EliminatedPlayerUnitsRemoved) {
    ReconnectionSetup setup;
    auto unitReg = makeRegistry();

    // Add two units for player A and one for player B.
    auto u1 = std::make_unique<game::Warrior>(1, 1, unitReg, setup.factionA);
    auto u2 = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    auto u3 = std::make_unique<game::Warrior>(3, 3, unitReg, setup.factionB);
    setup.session.mutableState().addUnit(std::move(u1));
    setup.session.mutableState().addUnit(std::move(u2));
    setup.session.mutableState().addUnit(std::move(u3));

    EXPECT_EQ(setup.session.getStateSnapshot().units().size(), 3);

    // Disconnect and wait for timeout.
    setup.manager.handleDisconnect(setup.playerA, setup.baseTime);
    auto checkTime = setup.baseTime + game::ReconnectionManager::DEFAULT_DISCONNECT_TIMEOUT;
    setup.manager.checkTimeouts(checkTime);

    // Only player B's unit should remain.
    const auto &units = setup.session.getStateSnapshot().units();
    EXPECT_EQ(units.size(), 1);
    EXPECT_EQ(units[0]->factionId(), setup.factionB);
}

} // namespace
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
