#pragma once

#include "game/GameSession.h"

#include <cstdint>
#include <unordered_set>

namespace game {

/// Configuration for turn timing and deadlines.
struct TurnConfig {
    /// Duration of a turn in seconds. Zero means no deadline.
    static constexpr double kDefaultTurnDurationSeconds = 60.0;

    /// Grace period in seconds after the first player ends their turn.
    /// Zero means no grace period (full deadline applies).
    static constexpr double kDefaultGracePeriodSeconds = 0.0;

    double turnDurationSeconds = kDefaultTurnDurationSeconds;
    double gracePeriodSeconds = kDefaultGracePeriodSeconds;
};

/// Manages simultaneous-turn timing on top of a GameSession.
///
/// All players act during the same turn window. The turn advances when every
/// connected player has ended their turn **or** when the deadline expires.
/// Disconnected players are automatically skipped.
///
/// TurnManager is a pure-logic wrapper: it does not own a clock. Callers
/// supply "current time" as a double (seconds since an arbitrary epoch) so that
/// the class remains fully deterministic and testable.
class TurnManager {
  public:
    /// Construct a TurnManager that wraps the given session.
    explicit TurnManager(GameSession &session, TurnConfig config = {});

    // -- Turn lifecycle ---------------------------------------------------

    /// Begin a new turn, computing the deadline from `currentTime`.
    /// Resets per-turn state (player-ready set, grace tracking).
    void startTurn(double currentTime);

    /// Mark a player as having ended their turn.
    /// If this is the first player to finish, the grace-period countdown
    /// starts (when configured).  Returns false if the player is unknown.
    bool playerEndTurn(PlayerId playerId, double currentTime);

    /// True when every connected player has ended their turn.
    [[nodiscard]] bool isReady() const;

    /// True when the hard deadline (or grace deadline) has passed.
    [[nodiscard]] bool isExpired(double currentTime) const;

    /// Attempt to advance the turn.  Succeeds when `isReady()` or
    /// `isExpired(currentTime)`.  On success the underlying GameSession
    /// resolves the turn and a new turn begins automatically.
    /// Returns true if the turn was advanced.
    bool tryAdvance(double currentTime);

    // -- Queries ----------------------------------------------------------

    /// Current turn number (delegates to the session).
    [[nodiscard]] int turnNumber() const;

    /// Absolute deadline for the current turn (seconds).
    [[nodiscard]] double deadline() const;

    /// Absolute grace deadline, or 0.0 if grace period is inactive.
    [[nodiscard]] double graceDeadline() const;

    /// Whether a specific player has ended the current turn.
    [[nodiscard]] bool hasPlayerEndedTurn(PlayerId playerId) const;

    /// Number of connected players who have ended the turn so far.
    [[nodiscard]] std::size_t readyPlayerCount() const;

    /// Read-only access to the configuration.
    [[nodiscard]] const TurnConfig &config() const;

    /// Mutable access for reconfiguration between turns.
    [[nodiscard]] TurnConfig &mutableConfig();

    /// Whether a turn is currently in progress.
    [[nodiscard]] bool isTurnInProgress() const;

  private:
    /// Check if all connected players are ready (delegates to session logic
    /// but also considers our own ready set for accuracy).
    [[nodiscard]] bool allConnectedPlayersReady() const;

    /// Force-end the turn for all connected players who haven't ended yet.
    /// Used when the deadline expires.
    void forceEndRemainingPlayers();

    GameSession *session_;
    TurnConfig config_;

    double turnStartTime_ = 0.0;
    double deadline_ = 0.0;
    double graceDeadline_ = 0.0;
    bool turnInProgress_ = false;
    bool graceActive_ = false;

    /// Players who have called playerEndTurn this turn.
    std::unordered_set<PlayerId> readyPlayers_;
};

} // namespace game
