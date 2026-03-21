#include "game/PredictionManager.h"

#include "game/AttackAction.h"
#include "game/CaptureAction.h"
#include "game/MoveAction.h"
#include "game/Unit.h"

#include <algorithm>
#include <ranges>
#include <variant>

namespace game {

PredictionManager::PredictionManager(GameSession &session) : session_(&session) {}

std::optional<SequenceId> PredictionManager::predictAction(PlayerId playerId, const GameAction &action) {
    // Capture unit state before applying the action.
    UnitSnapshot snapshot = captureSnapshot(action);

    // Try to apply the action on the local state.
    ActionResult result = session_->applyAction(playerId, action);
    if (result != ActionResult::Success) {
        return std::nullopt;
    }

    // Store the pending prediction.
    SequenceId seqId = nextSequenceId_++;
    pending_.push_back(PendingPrediction{
        .sequenceId = seqId, .action = action, .snapshot = snapshot, .status = PredictionStatus::Pending});

    return seqId;
}

bool PredictionManager::confirmAction(SequenceId sequenceId) {
    auto iter =
        std::ranges::find_if(pending_, [sequenceId](const PendingPrediction &p) { return p.sequenceId == sequenceId; });

    if (iter == pending_.end()) {
        return false;
    }

    pending_.erase(iter);
    return true;
}

bool PredictionManager::rejectAction(SequenceId sequenceId) {
    auto iter =
        std::ranges::find_if(pending_, [sequenceId](const PendingPrediction &p) { return p.sequenceId == sequenceId; });

    if (iter == pending_.end()) {
        return false;
    }

    // Rollback in reverse order from the rejected prediction to the newest,
    // then re-apply any predictions that come after the rejected one.
    // This ensures correct state when predictions depend on each other.

    // Collect predictions after the rejected one.
    std::vector<PendingPrediction> toReapply(std::next(iter), pending_.end());

    // Rollback predictions after the rejected one (newest first).
    for (const auto &pred : std::ranges::reverse_view(toReapply)) {
        rollbackPrediction(pred);
    }
    // Rollback the rejected prediction itself.
    rollbackPrediction(*iter);

    // Remove the rejected prediction and everything after it.
    pending_.erase(iter, pending_.end());

    // Re-apply the predictions that came after the rejected one.
    // Execute directly on state (bypassing player validation since these
    // were already validated when originally predicted).
    for (const auto &pred : toReapply) {
        bool success = std::visit(
            [this](const auto &act) -> bool {
                auto res = act.execute(session_->mutableState());
                return res.executed;
            },
            pred.action);

        if (success) {
            pending_.push_back(pred);
        }
        // If re-application fails, the prediction is dropped silently.
    }

    return true;
}

std::size_t PredictionManager::reconcile(const GameState &serverState) {
    // Save pending predictions.
    std::deque<PendingPrediction> savedPending = std::move(pending_);
    pending_.clear();

    // Replace local state with server state.
    // Copy turn, units, and map state from the server snapshot.
    GameState &localState = session_->mutableState();
    localState.setTurn(serverState.getTurn());

    // Copy unit positions and health from the server state.
    // For a full reconciliation, we need to sync the relevant parts of state.
    // In a real implementation this would do a deep copy; here we sync
    // unit-level data which is what predictions modify.
    const auto &serverUnits = serverState.units();
    auto &localUnits = localState.units();

    // Sync unit count and state.
    for (std::size_t i = 0; i < localUnits.size() && i < serverUnits.size(); ++i) {
        if (localUnits[i] && serverUnits[i]) {
            auto &lu = *localUnits[i];
            const auto &su = *serverUnits[i];

            // Unregister from old position.
            localState.mutableRegistry().unregisterUnit(lu.row(), lu.col(), &lu);

            // Sync state from server (direct position set, no movement cost).
            lu.setHealth(su.health());
            lu.setMovementRemaining(su.movementRemaining());
            lu.setPosition(su.row(), su.col());

            // Re-register at new position.
            localState.mutableRegistry().registerUnit(lu.row(), lu.col(), &lu);
        }
    }

    // Re-apply unconfirmed predictions on top of the server state.
    std::size_t reapplied = 0;
    for (const auto &pred : savedPending) {
        auto result = std::visit(
            [&localState](const auto &act) -> bool {
                auto res = act.execute(localState);
                return res.executed;
            },
            pred.action);

        if (result) {
            pending_.push_back(pred);
            ++reapplied;
        }
    }

    return reapplied;
}

std::size_t PredictionManager::pendingCount() const { return pending_.size(); }

SequenceId PredictionManager::nextSequenceId() const { return nextSequenceId_; }

bool PredictionManager::isPending(SequenceId sequenceId) const {
    return std::ranges::any_of(pending_,
                               [sequenceId](const PendingPrediction &p) { return p.sequenceId == sequenceId; });
}

void PredictionManager::clearPredictions() { pending_.clear(); }

UnitSnapshot PredictionManager::captureSnapshot(const GameAction &action) const {
    UnitSnapshot snapshot;
    const auto &units = session_->getStateSnapshot().units();

    std::size_t unitIndex = std::visit(
        [](const auto &act) -> std::size_t {
            using T = std::decay_t<decltype(act)>;
            if constexpr (std::is_same_v<T, AttackAction>) {
                return act.attackerIndex();
            } else {
                // MoveAction and CaptureAction both expose unitIndex().
                return act.unitIndex();
            }
        },
        action);

    snapshot.unitIndex = unitIndex;
    if (unitIndex < units.size() && units[unitIndex]) {
        const auto &unit = *units[unitIndex];
        snapshot.row = unit.row();
        snapshot.col = unit.col();
        snapshot.health = unit.health();
        snapshot.movementRemaining = unit.movementRemaining();
        snapshot.alive = unit.isAlive();
    }

    return snapshot;
}

void PredictionManager::rollbackPrediction(const PendingPrediction &prediction) {
    GameState &state = session_->mutableState();
    auto &units = state.units();
    const auto &snap = prediction.snapshot;

    if (snap.unitIndex < units.size() && units[snap.unitIndex]) {
        auto &unit = *units[snap.unitIndex];

        // Unregister from current position.
        state.mutableRegistry().unregisterUnit(unit.row(), unit.col(), &unit);

        // Restore the snapshot (direct position set, no movement cost).
        unit.setHealth(snap.health);
        unit.setMovementRemaining(snap.movementRemaining);
        unit.setPosition(snap.row, snap.col);

        // Re-register at original position.
        state.mutableRegistry().registerUnit(unit.row(), unit.col(), &unit);
    }
}

} // namespace game
