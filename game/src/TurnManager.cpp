#include "game/TurnManager.h"

namespace game {

TurnManager::TurnManager(GameSession &session, TurnConfig config) : session_(&session), config_(config) {}

// -- Turn lifecycle -------------------------------------------------------

void TurnManager::startTurn(double currentTime) {
    turnStartTime_ = currentTime;
    deadline_ = currentTime + config_.turnDurationSeconds;
    graceDeadline_ = 0.0;
    graceActive_ = false;
    turnInProgress_ = true;
    readyPlayers_.clear();
}

bool TurnManager::playerEndTurn(PlayerId playerId, double currentTime) {
    if (!session_->hasPlayer(playerId)) {
        return false;
    }

    // Delegate to the session so it tracks the endedTurn flag.
    session_->endTurn(playerId);
    readyPlayers_.insert(playerId);

    // Activate grace period on the first player finishing, if configured.
    if (!graceActive_ && config_.gracePeriodSeconds > 0.0) {
        graceActive_ = true;
        graceDeadline_ = currentTime + config_.gracePeriodSeconds;
    }

    return true;
}

bool TurnManager::isReady() const { return allConnectedPlayersReady(); }

bool TurnManager::isExpired(double currentTime) const {
    if (!turnInProgress_) {
        return false;
    }

    // Hard deadline always applies.
    if (currentTime >= deadline_) {
        return true;
    }

    // Grace deadline applies once active.
    if (graceActive_ && currentTime >= graceDeadline_) {
        return true;
    }

    return false;
}

bool TurnManager::tryAdvance(double currentTime) {
    if (!turnInProgress_) {
        return false;
    }

    if (!isReady() && !isExpired(currentTime)) {
        return false;
    }

    // Mark all connected players as having ended their turn so the session
    // considers the turn complete (covers the deadline-expired case where
    // some players never called endTurn).
    // The session's tryAdvanceTurn checks allPlayersEndedTurn(), so we need
    // to ensure disconnected players are already handled (they are, by
    // GameSession) and remaining connected players are marked.
    // We force-end any remaining connected players when the deadline expires.
    if (isExpired(currentTime) && !isReady()) {
        forceEndRemainingPlayers();
    }

    bool advanced = session_->tryAdvanceTurn();
    if (advanced) {
        // Immediately start the next turn.
        startTurn(currentTime);
    }
    return advanced;
}

// -- Queries --------------------------------------------------------------

int TurnManager::turnNumber() const { return session_->currentTurn(); }

double TurnManager::deadline() const { return deadline_; }

double TurnManager::graceDeadline() const { return graceDeadline_; }

bool TurnManager::hasPlayerEndedTurn(PlayerId playerId) const { return readyPlayers_.contains(playerId); }

std::size_t TurnManager::readyPlayerCount() const { return readyPlayers_.size(); }

const TurnConfig &TurnManager::config() const { return config_; }

TurnConfig &TurnManager::mutableConfig() { return config_; }

bool TurnManager::isTurnInProgress() const { return turnInProgress_; }

// -- Private helpers ------------------------------------------------------

bool TurnManager::allConnectedPlayersReady() const { return session_->allPlayersEndedTurn(); }

void TurnManager::forceEndRemainingPlayers() {
    for (PlayerId pid : session_->playerIds()) {
        const auto *info = session_->getPlayerInfo(pid);
        if (info != nullptr && info->status == PlayerStatus::Connected && !info->endedTurn) {
            session_->endTurn(pid);
        }
    }
}

} // namespace game
