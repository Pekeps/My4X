// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/TurnManager.h"

#include "game/GameSession.h"

#include <gtest/gtest.h>

namespace {

/// Helper to build a session with two connected players.
struct TurnManagerSetup {
    game::GameSession session{10, 10};
    game::PlayerId playerA = 1;
    game::PlayerId playerB = 2;

    TurnManagerSetup() {
        session.addPlayer(playerA, 1);
        session.addPlayer(playerB, 2);
    }
};

// ============================================================================
// Construction and configuration
// ============================================================================

TEST(TurnManager, DefaultConfig) {
    game::GameSession session(10, 10);
    game::TurnManager tm(session);
    EXPECT_EQ(tm.config().turnDurationSeconds, game::TurnConfig::kDefaultTurnDurationSeconds);
    EXPECT_EQ(tm.config().gracePeriodSeconds, game::TurnConfig::kDefaultGracePeriodSeconds);
}

TEST(TurnManager, CustomConfig) {
    game::GameSession session(10, 10);
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 120.0;
    cfg.gracePeriodSeconds = 15.0;
    game::TurnManager tm(session, cfg);
    EXPECT_EQ(tm.config().turnDurationSeconds, 120.0);
    EXPECT_EQ(tm.config().gracePeriodSeconds, 15.0);
}

TEST(TurnManager, MutableConfig) {
    game::GameSession session(10, 10);
    game::TurnManager tm(session);
    tm.mutableConfig().turnDurationSeconds = 90.0;
    EXPECT_EQ(tm.config().turnDurationSeconds, 90.0);
}

// ============================================================================
// startTurn
// ============================================================================

TEST(TurnManager, StartTurnSetsDeadline) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(100.0);
    EXPECT_EQ(tm.deadline(), 160.0);
    EXPECT_TRUE(tm.isTurnInProgress());
    EXPECT_EQ(tm.readyPlayerCount(), 0);
}

TEST(TurnManager, StartTurnResetsGrace) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    cfg.gracePeriodSeconds = 10.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    EXPECT_EQ(tm.graceDeadline(), 0.0);
}

// ============================================================================
// playerEndTurn
// ============================================================================

TEST(TurnManager, PlayerEndTurnSuccess) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    tm.startTurn(0.0);
    EXPECT_TRUE(tm.playerEndTurn(setup.playerA, 10.0));
    EXPECT_TRUE(tm.hasPlayerEndedTurn(setup.playerA));
    EXPECT_FALSE(tm.hasPlayerEndedTurn(setup.playerB));
    EXPECT_EQ(tm.readyPlayerCount(), 1);
}

TEST(TurnManager, PlayerEndTurnUnknownPlayerFails) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    tm.startTurn(0.0);
    EXPECT_FALSE(tm.playerEndTurn(99, 10.0));
}

TEST(TurnManager, PlayerEndTurnActivatesGrace) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    cfg.gracePeriodSeconds = 10.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    EXPECT_EQ(tm.graceDeadline(), 0.0);

    tm.playerEndTurn(setup.playerA, 25.0);
    EXPECT_EQ(tm.graceDeadline(), 35.0);
}

TEST(TurnManager, GraceOnlyActivatesOnFirstPlayer) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    cfg.gracePeriodSeconds = 10.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    tm.playerEndTurn(setup.playerA, 25.0);
    EXPECT_EQ(tm.graceDeadline(), 35.0);

    // Second player ending should NOT change the grace deadline.
    tm.playerEndTurn(setup.playerB, 30.0);
    EXPECT_EQ(tm.graceDeadline(), 35.0);
}

// ============================================================================
// isReady
// ============================================================================

TEST(TurnManager, IsReadyWhenAllPlayersEndTurn) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    tm.startTurn(0.0);
    EXPECT_FALSE(tm.isReady());

    tm.playerEndTurn(setup.playerA, 5.0);
    EXPECT_FALSE(tm.isReady());

    tm.playerEndTurn(setup.playerB, 10.0);
    EXPECT_TRUE(tm.isReady());
}

TEST(TurnManager, DisconnectedPlayersSkipped) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    setup.session.disconnectPlayer(setup.playerB);
    tm.startTurn(0.0);

    tm.playerEndTurn(setup.playerA, 5.0);
    EXPECT_TRUE(tm.isReady());
}

TEST(TurnManager, AllDisconnectedIsReadyForSession) {
    // Edge case: if all players disconnect, session's allPlayersEndedTurn
    // returns true because it skips disconnected players.
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    setup.session.disconnectPlayer(setup.playerA);
    setup.session.disconnectPlayer(setup.playerB);
    tm.startTurn(0.0);

    // With the session's logic, all disconnected -> allPlayersEndedTurn returns true
    // (because the ranges::all_of passes vacuously for the connected filter).
    // Actually no: session returns false if players_ is empty, but it's not empty,
    // just disconnected. Let's verify:
    EXPECT_TRUE(tm.isReady());
}

// ============================================================================
// isExpired
// ============================================================================

TEST(TurnManager, IsExpiredBeforeDeadline) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    EXPECT_FALSE(tm.isExpired(30.0));
    EXPECT_FALSE(tm.isExpired(59.9));
}

TEST(TurnManager, IsExpiredAtDeadline) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    EXPECT_TRUE(tm.isExpired(60.0));
}

TEST(TurnManager, IsExpiredAfterDeadline) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    EXPECT_TRUE(tm.isExpired(100.0));
}

TEST(TurnManager, IsExpiredWhenNoTurnInProgress) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);
    // No turn started — should never be expired.
    EXPECT_FALSE(tm.isExpired(99999.0));
}

TEST(TurnManager, GraceDeadlineExpiresBeforeHardDeadline) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    cfg.gracePeriodSeconds = 10.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    tm.playerEndTurn(setup.playerA, 5.0);
    // Grace deadline = 5.0 + 10.0 = 15.0

    EXPECT_FALSE(tm.isExpired(10.0));
    EXPECT_TRUE(tm.isExpired(15.0));
    EXPECT_TRUE(tm.isExpired(20.0));
}

// ============================================================================
// tryAdvance — all players ready
// ============================================================================

TEST(TurnManager, TryAdvanceSucceedsWhenAllReady) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    tm.startTurn(0.0);
    EXPECT_EQ(tm.turnNumber(), 1);

    tm.playerEndTurn(setup.playerA, 5.0);
    tm.playerEndTurn(setup.playerB, 10.0);

    EXPECT_TRUE(tm.tryAdvance(10.0));
    EXPECT_EQ(tm.turnNumber(), 2);
}

TEST(TurnManager, TryAdvanceFailsWhenNotReady) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    tm.startTurn(0.0);
    tm.playerEndTurn(setup.playerA, 5.0);

    EXPECT_FALSE(tm.tryAdvance(5.0));
    EXPECT_EQ(tm.turnNumber(), 1);
}

// ============================================================================
// tryAdvance — deadline expired
// ============================================================================

TEST(TurnManager, TryAdvanceOnDeadlineExpiry) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    // No players end turn, but deadline passes.
    EXPECT_TRUE(tm.tryAdvance(60.0));
    EXPECT_EQ(tm.turnNumber(), 2);
}

TEST(TurnManager, TryAdvanceOnGraceExpiry) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    cfg.gracePeriodSeconds = 10.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    tm.playerEndTurn(setup.playerA, 5.0);
    // Grace deadline = 15.0, hard deadline = 60.0

    // Before grace — not ready, not expired.
    EXPECT_FALSE(tm.tryAdvance(10.0));

    // At grace deadline — should advance.
    EXPECT_TRUE(tm.tryAdvance(15.0));
    EXPECT_EQ(tm.turnNumber(), 2);
}

// ============================================================================
// tryAdvance — disconnected players
// ============================================================================

TEST(TurnManager, DisconnectedPlayersAutoSkippedOnAdvance) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    setup.session.disconnectPlayer(setup.playerB);
    tm.startTurn(0.0);

    tm.playerEndTurn(setup.playerA, 5.0);
    EXPECT_TRUE(tm.tryAdvance(5.0));
    EXPECT_EQ(tm.turnNumber(), 2);
}

TEST(TurnManager, DeadlineForceEndsRemainingPlayers) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 30.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    // Only player A ends turn. Player B does not.
    tm.playerEndTurn(setup.playerA, 10.0);

    // Before deadline — cannot advance.
    EXPECT_FALSE(tm.tryAdvance(20.0));

    // At deadline — force-ends player B and advances.
    EXPECT_TRUE(tm.tryAdvance(30.0));
    EXPECT_EQ(tm.turnNumber(), 2);
}

// ============================================================================
// tryAdvance — no turn in progress
// ============================================================================

TEST(TurnManager, TryAdvanceFailsWhenNoTurnInProgress) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);
    // No turn started.
    EXPECT_FALSE(tm.tryAdvance(0.0));
}

// ============================================================================
// Auto-restart after advance
// ============================================================================

TEST(TurnManager, TurnAutoRestartsAfterAdvance) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    tm.playerEndTurn(setup.playerA, 10.0);
    tm.playerEndTurn(setup.playerB, 20.0);
    EXPECT_TRUE(tm.tryAdvance(20.0));

    // After advance, a new turn should be in progress with a fresh deadline.
    EXPECT_TRUE(tm.isTurnInProgress());
    EXPECT_EQ(tm.deadline(), 80.0); // 20.0 + 60.0
    EXPECT_EQ(tm.readyPlayerCount(), 0);
    EXPECT_FALSE(tm.hasPlayerEndedTurn(setup.playerA));
    EXPECT_FALSE(tm.hasPlayerEndedTurn(setup.playerB));
}

// ============================================================================
// Multi-turn sequence
// ============================================================================

TEST(TurnManager, MultiTurnSequence) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 60.0;
    game::TurnManager tm(setup.session, cfg);

    double time = 0.0;
    tm.startTurn(time);

    for (int turn = 1; turn <= 5; ++turn) {
        EXPECT_EQ(tm.turnNumber(), turn);
        time += 10.0;
        tm.playerEndTurn(setup.playerA, time);
        time += 5.0;
        tm.playerEndTurn(setup.playerB, time);
        EXPECT_TRUE(tm.tryAdvance(time));
    }
    EXPECT_EQ(tm.turnNumber(), 6);
}

// ============================================================================
// Configurable turn duration
// ============================================================================

TEST(TurnManager, ConfigurableTurnDuration) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 120.0;
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    EXPECT_EQ(tm.deadline(), 120.0);

    // At 60 seconds, not expired yet.
    EXPECT_FALSE(tm.isExpired(60.0));

    // At 120, expired.
    EXPECT_TRUE(tm.isExpired(120.0));
}

// ============================================================================
// Reconnected player must end turn
// ============================================================================

TEST(TurnManager, ReconnectedPlayerMustEndTurn) {
    TurnManagerSetup setup;
    game::TurnManager tm(setup.session);

    // Disconnect B, start turn, then reconnect B.
    setup.session.disconnectPlayer(setup.playerB);
    tm.startTurn(0.0);
    setup.session.connectPlayer(setup.playerB);

    // A ends turn — not ready because B is now connected.
    tm.playerEndTurn(setup.playerA, 5.0);
    EXPECT_FALSE(tm.isReady());

    // B ends turn — now ready.
    tm.playerEndTurn(setup.playerB, 10.0);
    EXPECT_TRUE(tm.isReady());
    EXPECT_TRUE(tm.tryAdvance(10.0));
}

// ============================================================================
// turnNumber delegates to session
// ============================================================================

TEST(TurnManager, TurnNumberDelegatesToSession) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    game::TurnManager tm(session);

    EXPECT_EQ(tm.turnNumber(), 1);

    // Manually advance via session to verify delegation.
    session.endTurn(1);
    session.tryAdvanceTurn();
    EXPECT_EQ(tm.turnNumber(), 2);
}

// ============================================================================
// Single-player scenarios
// ============================================================================

TEST(TurnManager, SinglePlayerTurn) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    game::TurnManager tm(session);

    tm.startTurn(0.0);
    tm.playerEndTurn(1, 5.0);
    EXPECT_TRUE(tm.isReady());
    EXPECT_TRUE(tm.tryAdvance(5.0));
    EXPECT_EQ(tm.turnNumber(), 2);
}

// ============================================================================
// Edge: deadline with grace period longer than turn duration
// ============================================================================

TEST(TurnManager, GracePeriodLongerThanTurnDuration) {
    TurnManagerSetup setup;
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 30.0;
    cfg.gracePeriodSeconds = 60.0; // Grace is longer than the turn itself
    game::TurnManager tm(setup.session, cfg);

    tm.startTurn(0.0);
    tm.playerEndTurn(setup.playerA, 5.0);
    // Grace deadline = 65.0, hard deadline = 30.0

    // At 30 seconds, hard deadline kicks in first.
    EXPECT_TRUE(tm.isExpired(30.0));
    EXPECT_TRUE(tm.tryAdvance(30.0));
}

// ============================================================================
// Three-player scenarios
// ============================================================================

TEST(TurnManager, ThreePlayersOneDisconnected) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    session.addPlayer(2, 2);
    session.addPlayer(3, 3);
    game::TurnManager tm(session);

    session.disconnectPlayer(2);
    tm.startTurn(0.0);

    tm.playerEndTurn(1, 5.0);
    EXPECT_FALSE(tm.isReady());

    tm.playerEndTurn(3, 10.0);
    EXPECT_TRUE(tm.isReady());
    EXPECT_TRUE(tm.tryAdvance(10.0));
    EXPECT_EQ(tm.turnNumber(), 2);
}

TEST(TurnManager, ThreePlayersDeadlineExpiresWithOneReady) {
    game::GameSession session(10, 10);
    session.addPlayer(1, 1);
    session.addPlayer(2, 2);
    session.addPlayer(3, 3);
    game::TurnConfig cfg;
    cfg.turnDurationSeconds = 30.0;
    game::TurnManager tm(session, cfg);

    tm.startTurn(0.0);
    tm.playerEndTurn(1, 10.0);

    // Deadline expires — players 2 and 3 are force-ended.
    EXPECT_TRUE(tm.tryAdvance(30.0));
    EXPECT_EQ(tm.turnNumber(), 2);
}

} // namespace
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
