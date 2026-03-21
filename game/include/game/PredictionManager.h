#pragma once

#include "game/GameSession.h"
#include "game/GameState.h"

#include <cstdint>
#include <deque>
#include <optional>

namespace game {

/// Unique identifier for a predicted action.
using SequenceId = std::uint32_t;

/// Status of a prediction in its lifecycle.
enum class PredictionStatus : std::uint8_t {
    Pending,
    Confirmed,
    Rejected,
};

/// Snapshot of the unit state before a predicted action is applied.
/// Used to rollback when the server rejects a prediction.
struct UnitSnapshot {
    std::size_t unitIndex = 0;
    int row = 0;
    int col = 0;
    int health = 0;
    int movementRemaining = 0;
    bool alive = true;
};

/// A single pending prediction entry: the action, its sequence number, and the
/// pre-action state needed for rollback.
struct PendingPrediction {
    SequenceId sequenceId = 0;
    GameAction action;
    UnitSnapshot snapshot;
    PredictionStatus status = PredictionStatus::Pending;
};

/// Client-side prediction manager for optimistic action application.
///
/// In a networked turn-based 4X game, this class lets the client apply
/// actions locally before the server confirms them, then reconciles with
/// the server's authoritative response.
///
/// Lifecycle:
///   1. predictAction() — apply locally, store pending prediction
///   2. Server processes and responds with confirm or reject
///   3. confirmAction(seqId) — remove from pending
///      OR rejectAction(seqId) — rollback and remove
///   4. reconcile(serverState) — full state reconciliation
class PredictionManager {
  public:
    /// Construct a PredictionManager operating on the given session.
    explicit PredictionManager(GameSession &session);

    /// Apply an action optimistically on the local game state.
    /// Returns the sequence ID assigned to this prediction, or std::nullopt
    /// if the action failed to apply locally.
    std::optional<SequenceId> predictAction(PlayerId playerId, const GameAction &action);

    /// Confirm that the server accepted the action with the given sequence ID.
    /// Removes the prediction from the pending queue.
    /// Returns true if the prediction was found and confirmed.
    bool confirmAction(SequenceId sequenceId);

    /// Reject the action with the given sequence ID.
    /// Rolls back the local state change and removes the prediction.
    /// Returns true if the prediction was found and rolled back.
    bool rejectAction(SequenceId sequenceId);

    /// Full state reconciliation: replace local state with the server's
    /// authoritative state, then re-apply any remaining unconfirmed predictions.
    /// Returns the number of predictions that were successfully re-applied.
    std::size_t reconcile(const GameState &serverState);

    /// Return the number of pending (unconfirmed) predictions.
    [[nodiscard]] std::size_t pendingCount() const;

    /// Return the current sequence counter (next ID to be assigned).
    [[nodiscard]] SequenceId nextSequenceId() const;

    /// Check whether a specific sequence ID is still pending.
    [[nodiscard]] bool isPending(SequenceId sequenceId) const;

    /// Clear all pending predictions without rolling back.
    void clearPredictions();

  private:
    /// Capture a snapshot of the unit involved in the given action.
    [[nodiscard]] UnitSnapshot captureSnapshot(const GameAction &action) const;

    /// Rollback a single prediction by restoring the unit snapshot.
    void rollbackPrediction(const PendingPrediction &prediction);

    /// Initial sequence ID value.
    static constexpr SequenceId INITIAL_SEQUENCE_ID = 1;

    GameSession *session_;
    SequenceId nextSequenceId_ = INITIAL_SEQUENCE_ID;
    std::deque<PendingPrediction> pending_;
};

} // namespace game
