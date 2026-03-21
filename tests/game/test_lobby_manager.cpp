// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/LobbyManager.h"

#include "game/GameSession.h"

#include <gtest/gtest.h>

namespace {

// ============================================================================
// Game creation tests
// ============================================================================

TEST(LobbyManager, CreateGameReturnsUniqueId) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.gameName = "Game1";

    game::GameId id1 = mgr.createGame(1, settings);
    game::GameId id2 = mgr.createGame(2, settings);

    EXPECT_NE(id1, id2);
    EXPECT_EQ(mgr.gameCount(), 2);
}

TEST(LobbyManager, CreateGameSetsHostAsFirstPlayer) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.gameName = "HostGame";

    game::GameId id = mgr.createGame(42, settings);
    auto info = mgr.getGameInfo(id);

    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->hostPlayerId, 42);
    EXPECT_EQ(info->state, game::LobbyGameState::WaitingForPlayers);
    EXPECT_EQ(info->gameName, "HostGame");

    // Host should occupy the first slot.
    ASSERT_FALSE(info->playerSlots.empty());
    EXPECT_TRUE(info->playerSlots[0].occupied);
    EXPECT_EQ(info->playerSlots[0].playerId, 42);
}

TEST(LobbyManager, CreateGameWithSettings) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.mapSize = game::MapSize::Large;
    settings.maxPlayers = 4;
    settings.minPlayers = 2;
    settings.turnDeadlineSeconds = 120;
    settings.randomSeed = 12345;
    settings.gameName = "Custom Game";

    game::GameId id = mgr.createGame(1, settings);
    auto info = mgr.getGameInfo(id);

    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->settings.mapSize, game::MapSize::Large);
    EXPECT_EQ(info->settings.maxPlayers, 4);
    EXPECT_EQ(info->settings.minPlayers, 2);
    EXPECT_EQ(info->settings.turnDeadlineSeconds, 120);
    EXPECT_EQ(info->settings.randomSeed, 12345);
    EXPECT_EQ(info->settings.gameName, "Custom Game");
}

// ============================================================================
// GameSettings map dimension tests
// ============================================================================

TEST(GameSettings, SmallMapDimensions) {
    game::GameSettings settings;
    settings.mapSize = game::MapSize::Small;
    EXPECT_EQ(settings.mapRows(), game::GameSettings::SMALL_MAP_ROWS);
    EXPECT_EQ(settings.mapCols(), game::GameSettings::SMALL_MAP_COLS);
}

TEST(GameSettings, MediumMapDimensions) {
    game::GameSettings settings;
    settings.mapSize = game::MapSize::Medium;
    EXPECT_EQ(settings.mapRows(), game::GameSettings::MEDIUM_MAP_ROWS);
    EXPECT_EQ(settings.mapCols(), game::GameSettings::MEDIUM_MAP_COLS);
}

TEST(GameSettings, LargeMapDimensions) {
    game::GameSettings settings;
    settings.mapSize = game::MapSize::Large;
    EXPECT_EQ(settings.mapRows(), game::GameSettings::LARGE_MAP_ROWS);
    EXPECT_EQ(settings.mapCols(), game::GameSettings::LARGE_MAP_COLS);
}

// ============================================================================
// List games tests
// ============================================================================

TEST(LobbyManager, ListGamesReturnsAll) {
    game::LobbyManager mgr;
    game::GameSettings settings;

    mgr.createGame(1, settings);
    mgr.createGame(2, settings);
    mgr.createGame(3, settings);

    auto all = mgr.listGames();
    EXPECT_EQ(all.size(), 3);
}

TEST(LobbyManager, ListGamesFilterByState) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id1 = mgr.createGame(1, settings);
    mgr.createGame(2, settings);

    // Start game 1 so it transitions to InProgress.
    mgr.startGame(id1, 1);

    auto waiting = mgr.listGames(game::LobbyGameState::WaitingForPlayers);
    EXPECT_EQ(waiting.size(), 1);

    auto inProgress = mgr.listGames(game::LobbyGameState::InProgress);
    EXPECT_EQ(inProgress.size(), 1);
}

TEST(LobbyManager, ListGamesReturnsCorrectSummary) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.gameName = "Test Game";
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    mgr.joinGame(id, 2);

    auto all = mgr.listGames();
    ASSERT_EQ(all.size(), 1);
    EXPECT_EQ(all[0].gameId, id);
    EXPECT_EQ(all[0].gameName, "Test Game");
    EXPECT_EQ(all[0].state, game::LobbyGameState::WaitingForPlayers);
    EXPECT_EQ(all[0].currentPlayers, 2);
    EXPECT_EQ(all[0].maxPlayers, 4);
}

// ============================================================================
// Join game tests
// ============================================================================

TEST(LobbyManager, JoinGameSucceeds) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    EXPECT_TRUE(mgr.joinGame(id, 2));
    EXPECT_TRUE(mgr.joinGame(id, 3));

    auto info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());

    // Count occupied slots.
    int occupied = 0;
    for (const auto &slot : info->playerSlots) {
        if (slot.occupied) {
            ++occupied;
        }
    }
    EXPECT_EQ(occupied, 3); // host + 2 joiners
}

TEST(LobbyManager, JoinGameFailsWhenFull) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.maxPlayers = 2;

    game::GameId id = mgr.createGame(1, settings); // host fills slot 1
    EXPECT_TRUE(mgr.joinGame(id, 2));              // fills slot 2
    EXPECT_FALSE(mgr.joinGame(id, 3));             // full
}

TEST(LobbyManager, JoinGameFailsIfNotWaiting) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id = mgr.createGame(1, settings);
    mgr.startGame(id, 1);

    EXPECT_FALSE(mgr.joinGame(id, 2));
}

TEST(LobbyManager, JoinGameFailsIfAlreadyJoined) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    EXPECT_TRUE(mgr.joinGame(id, 2));
    EXPECT_FALSE(mgr.joinGame(id, 2)); // duplicate
}

TEST(LobbyManager, JoinGameFailsIfNotFound) {
    game::LobbyManager mgr;
    EXPECT_FALSE(mgr.joinGame(999, 1));
}

TEST(LobbyManager, HostCannotJoinOwnGame) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    EXPECT_FALSE(mgr.joinGame(id, 1)); // host already in the game
}

// ============================================================================
// Leave game tests
// ============================================================================

TEST(LobbyManager, LeaveGameNonHostFreesSlot) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    mgr.joinGame(id, 2);

    EXPECT_TRUE(mgr.leaveGame(id, 2));

    // Game should still exist.
    EXPECT_TRUE(mgr.hasGame(id));

    auto info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());

    // Only host should remain.
    int occupied = 0;
    for (const auto &slot : info->playerSlots) {
        if (slot.occupied) {
            ++occupied;
        }
    }
    EXPECT_EQ(occupied, 1);
}

TEST(LobbyManager, LeaveGameHostRemovesGame) {
    game::LobbyManager mgr;
    game::GameSettings settings;

    game::GameId id = mgr.createGame(1, settings);
    mgr.joinGame(id, 2);

    EXPECT_TRUE(mgr.leaveGame(id, 1)); // host leaves
    EXPECT_FALSE(mgr.hasGame(id));     // game removed
}

TEST(LobbyManager, LeaveGameFailsIfNotFound) {
    game::LobbyManager mgr;
    EXPECT_FALSE(mgr.leaveGame(999, 1));
}

TEST(LobbyManager, LeaveGameFailsIfPlayerNotInGame) {
    game::LobbyManager mgr;
    game::GameSettings settings;

    game::GameId id = mgr.createGame(1, settings);
    EXPECT_FALSE(mgr.leaveGame(id, 99)); // player 99 not in game
}

TEST(LobbyManager, LeaveGameFailsIfInProgress) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id = mgr.createGame(1, settings);
    mgr.startGame(id, 1);

    EXPECT_FALSE(mgr.leaveGame(id, 1));
}

// ============================================================================
// Get game info tests
// ============================================================================

TEST(LobbyManager, GetGameInfoNotFound) {
    game::LobbyManager mgr;
    auto info = mgr.getGameInfo(999);
    EXPECT_FALSE(info.has_value());
}

TEST(LobbyManager, GetGameInfoReturnsDetails) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.gameName = "Detailed";
    settings.maxPlayers = 6;

    game::GameId id = mgr.createGame(10, settings);
    mgr.joinGame(id, 20);

    auto info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->gameId, id);
    EXPECT_EQ(info->gameName, "Detailed");
    EXPECT_EQ(info->hostPlayerId, 10);
    EXPECT_EQ(info->playerSlots.size(), 6);
}

// ============================================================================
// Start game tests
// ============================================================================

TEST(LobbyManager, StartGameSucceeds) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.mapSize = game::MapSize::Small;
    settings.minPlayers = 2;
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    mgr.joinGame(id, 2);

    auto *session = mgr.startGame(id, 1);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->playerCount(), 2);

    auto info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, game::LobbyGameState::InProgress);
}

TEST(LobbyManager, StartGameFailsIfNotHost) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 2;
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    mgr.joinGame(id, 2);

    auto *session = mgr.startGame(id, 2); // not host
    EXPECT_EQ(session, nullptr);
}

TEST(LobbyManager, StartGameFailsIfNotEnoughPlayers) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 3;
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    mgr.joinGame(id, 2); // only 2 players, need 3

    auto *session = mgr.startGame(id, 1);
    EXPECT_EQ(session, nullptr);

    auto info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, game::LobbyGameState::WaitingForPlayers);
}

TEST(LobbyManager, StartGameFailsIfAlreadyInProgress) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id = mgr.createGame(1, settings);
    ASSERT_NE(mgr.startGame(id, 1), nullptr);

    // Trying to start again should fail.
    EXPECT_EQ(mgr.startGame(id, 1), nullptr);
}

TEST(LobbyManager, StartGameFailsIfNotFound) {
    game::LobbyManager mgr;
    EXPECT_EQ(mgr.startGame(999, 1), nullptr);
}

TEST(LobbyManager, StartGameCreatesPlayableSession) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.mapSize = game::MapSize::Small;
    settings.minPlayers = 2;
    settings.maxPlayers = 2;

    game::GameId id = mgr.createGame(1, settings);
    mgr.joinGame(id, 2);

    auto *session = mgr.startGame(id, 1);
    ASSERT_NE(session, nullptr);

    // Verify the session is functional.
    EXPECT_EQ(session->currentTurn(), 1);
    EXPECT_TRUE(session->hasPlayer(1));
    EXPECT_TRUE(session->hasPlayer(2));

    // Players can end turns.
    EXPECT_TRUE(session->endTurn(1));
    EXPECT_TRUE(session->endTurn(2));
    EXPECT_TRUE(session->tryAdvanceTurn());
    EXPECT_EQ(session->currentTurn(), 2);
}

// ============================================================================
// Finish game tests
// ============================================================================

TEST(LobbyManager, FinishGameSucceeds) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id = mgr.createGame(1, settings);
    mgr.startGame(id, 1);

    EXPECT_TRUE(mgr.finishGame(id));

    auto info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, game::LobbyGameState::Finished);
}

TEST(LobbyManager, FinishGameFailsIfNotInProgress) {
    game::LobbyManager mgr;
    game::GameSettings settings;

    game::GameId id = mgr.createGame(1, settings);
    EXPECT_FALSE(mgr.finishGame(id)); // still waiting
}

TEST(LobbyManager, FinishGameFailsIfNotFound) {
    game::LobbyManager mgr;
    EXPECT_FALSE(mgr.finishGame(999));
}

// ============================================================================
// GetSession tests
// ============================================================================

TEST(LobbyManager, GetSessionReturnsNullForWaiting) {
    game::LobbyManager mgr;
    game::GameSettings settings;

    game::GameId id = mgr.createGame(1, settings);
    EXPECT_EQ(mgr.getSession(id), nullptr);
}

TEST(LobbyManager, GetSessionReturnsSessionForInProgress) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id = mgr.createGame(1, settings);
    mgr.startGame(id, 1);

    EXPECT_NE(mgr.getSession(id), nullptr);
}

TEST(LobbyManager, GetSessionReturnsNullForFinished) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id = mgr.createGame(1, settings);
    mgr.startGame(id, 1);
    mgr.finishGame(id);

    EXPECT_EQ(mgr.getSession(id), nullptr);
}

// ============================================================================
// Game state machine tests
// ============================================================================

TEST(LobbyManager, GameStateTransitions) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id = mgr.createGame(1, settings);

    // Initial state: WaitingForPlayers.
    auto info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, game::LobbyGameState::WaitingForPlayers);

    // Transition to InProgress.
    mgr.startGame(id, 1);
    info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, game::LobbyGameState::InProgress);

    // Transition to Finished.
    mgr.finishGame(id);
    info = mgr.getGameInfo(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, game::LobbyGameState::Finished);
}

// ============================================================================
// Lobby lifecycle integration test
// ============================================================================

TEST(LobbyManager, FullLifecycle) {
    game::LobbyManager mgr;

    // Create a game.
    game::GameSettings settings;
    settings.mapSize = game::MapSize::Small;
    settings.maxPlayers = 3;
    settings.minPlayers = 2;
    settings.gameName = "Epic Battle";

    game::GameId id = mgr.createGame(100, settings);

    // List shows 1 waiting game.
    auto waiting = mgr.listGames(game::LobbyGameState::WaitingForPlayers);
    EXPECT_EQ(waiting.size(), 1);
    EXPECT_EQ(waiting[0].gameName, "Epic Battle");
    EXPECT_EQ(waiting[0].currentPlayers, 1);

    // Player 200 joins.
    EXPECT_TRUE(mgr.joinGame(id, 200));
    waiting = mgr.listGames(game::LobbyGameState::WaitingForPlayers);
    EXPECT_EQ(waiting[0].currentPlayers, 2);

    // Player 300 joins, then leaves.
    EXPECT_TRUE(mgr.joinGame(id, 300));
    EXPECT_TRUE(mgr.leaveGame(id, 300));
    waiting = mgr.listGames(game::LobbyGameState::WaitingForPlayers);
    EXPECT_EQ(waiting[0].currentPlayers, 2);

    // Slot freed by 300 can be reused.
    EXPECT_TRUE(mgr.joinGame(id, 400));

    // Host starts the game (3 players: 100, 200, 400).
    auto *session = mgr.startGame(id, 100);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->playerCount(), 3);

    // Game is now in progress.
    auto inProgress = mgr.listGames(game::LobbyGameState::InProgress);
    EXPECT_EQ(inProgress.size(), 1);

    // Session is accessible.
    EXPECT_NE(mgr.getSession(id), nullptr);

    // Finish the game.
    EXPECT_TRUE(mgr.finishGame(id));
    auto finished = mgr.listGames(game::LobbyGameState::Finished);
    EXPECT_EQ(finished.size(), 1);
}

TEST(LobbyManager, MultipleGamesIndependent) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.minPlayers = 1;

    game::GameId id1 = mgr.createGame(1, settings);
    game::GameId id2 = mgr.createGame(2, settings);

    // Start game 1, leave game 2 waiting.
    ASSERT_NE(mgr.startGame(id1, 1), nullptr);

    EXPECT_EQ(mgr.getGameInfo(id1)->state, game::LobbyGameState::InProgress);
    EXPECT_EQ(mgr.getGameInfo(id2)->state, game::LobbyGameState::WaitingForPlayers);

    // Finish game 1.
    EXPECT_TRUE(mgr.finishGame(id1));
    EXPECT_EQ(mgr.getGameInfo(id1)->state, game::LobbyGameState::Finished);
    EXPECT_EQ(mgr.getGameInfo(id2)->state, game::LobbyGameState::WaitingForPlayers);
}

TEST(LobbyManager, HasGameAndGameCount) {
    game::LobbyManager mgr;
    EXPECT_EQ(mgr.gameCount(), 0);
    EXPECT_FALSE(mgr.hasGame(1));

    game::GameSettings settings;
    game::GameId id = mgr.createGame(1, settings);
    EXPECT_EQ(mgr.gameCount(), 1);
    EXPECT_TRUE(mgr.hasGame(id));
}

TEST(LobbyManager, RejoinAfterLeave) {
    game::LobbyManager mgr;
    game::GameSettings settings;
    settings.maxPlayers = 4;

    game::GameId id = mgr.createGame(1, settings);
    EXPECT_TRUE(mgr.joinGame(id, 2));
    EXPECT_TRUE(mgr.leaveGame(id, 2));
    EXPECT_TRUE(mgr.joinGame(id, 2)); // should succeed after leaving
}

} // namespace
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
