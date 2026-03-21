#include "game/ReconnectionManager.h"

#include "game/Unit.h"

#include <algorithm>

namespace game {

ReconnectionManager::ReconnectionManager(GameSession &session, std::chrono::steady_clock::duration timeout)
    : session_(&session), timeout_(timeout) {}

// -- Player registration ---------------------------------------------------

bool ReconnectionManager::registerPlayer(PlayerId playerId, FactionId factionId) {
    if (players_.contains(playerId)) {
        return false;
    }
    PlayerConnectionInfo info;
    info.playerId = playerId;
    info.factionId = factionId;
    info.connected = true;
    players_[playerId] = info;
    return true;
}

bool ReconnectionManager::unregisterPlayer(PlayerId playerId) { return players_.erase(playerId) > 0; }

// -- Disconnect / Reconnect ------------------------------------------------

bool ReconnectionManager::handleDisconnect(PlayerId playerId, std::chrono::steady_clock::time_point now) {
    auto iter = players_.find(playerId);
    if (iter == players_.end()) {
        return false;
    }
    if (!iter->second.connected) {
        return false; // already disconnected
    }
    iter->second.connected = false;
    iter->second.disconnectTime = now;

    // Propagate to the underlying session.
    session_->disconnectPlayer(playerId);
    return true;
}

ReconnectResult ReconnectionManager::handleReconnect(PlayerId playerId, FactionId factionId,
                                                     std::chrono::steady_clock::time_point now) {
    // Already eliminated?
    if (eliminated_.contains(playerId)) {
        return ReconnectResult::TimedOut;
    }

    auto iter = players_.find(playerId);
    if (iter == players_.end()) {
        return ReconnectResult::UnknownPlayer;
    }

    // Identity check: faction must match.
    if (iter->second.factionId != factionId) {
        return ReconnectResult::UnknownPlayer;
    }

    if (iter->second.connected) {
        return ReconnectResult::AlreadyConnected;
    }

    // Check timeout before allowing reconnection.
    auto elapsed = now - iter->second.disconnectTime;
    if (elapsed >= timeout_) {
        // Eliminate the player on the spot.
        eliminatePlayerUnits(iter->second.factionId);
        session_->removePlayer(playerId);
        players_.erase(iter);
        eliminated_.insert(playerId);
        return ReconnectResult::TimedOut;
    }

    // Reconnect: mark as connected.
    iter->second.connected = true;
    session_->connectPlayer(playerId);

    // Generate a full state snapshot for the reconnecting client.
    lastSnapshot_ = serializeGameState(session_->getStateSnapshot());

    return ReconnectResult::Success;
}

const std::optional<std::string> &ReconnectionManager::lastSnapshot() const { return lastSnapshot_; }

// -- Timeout management ----------------------------------------------------

std::vector<PlayerId> ReconnectionManager::checkTimeouts(std::chrono::steady_clock::time_point now) {
    std::vector<PlayerId> timedOut;

    for (auto iter = players_.begin(); iter != players_.end();) {
        if (!iter->second.connected) {
            auto elapsed = now - iter->second.disconnectTime;
            if (elapsed >= timeout_) {
                PlayerId pid = iter->first;
                FactionId fid = iter->second.factionId;
                timedOut.push_back(pid);
                eliminatePlayerUnits(fid);
                session_->removePlayer(pid);
                eliminated_.insert(pid);
                iter = players_.erase(iter);
                continue;
            }
        }
        ++iter;
    }
    return timedOut;
}

bool ReconnectionManager::isEliminated(PlayerId playerId) const { return eliminated_.contains(playerId); }

// -- Query -----------------------------------------------------------------

const PlayerConnectionInfo *ReconnectionManager::getConnectionInfo(PlayerId playerId) const {
    auto iter = players_.find(playerId);
    if (iter == players_.end()) {
        return nullptr;
    }
    return &iter->second;
}

std::size_t ReconnectionManager::trackedPlayerCount() const { return players_.size(); }

std::size_t ReconnectionManager::connectedPlayerCount() const {
    return static_cast<std::size_t>(
        std::ranges::count_if(players_, [](const auto &entry) { return entry.second.connected; }));
}

std::size_t ReconnectionManager::disconnectedPlayerCount() const {
    return static_cast<std::size_t>(
        std::ranges::count_if(players_, [](const auto &entry) { return !entry.second.connected; }));
}

std::chrono::steady_clock::duration ReconnectionManager::disconnectTimeout() const { return timeout_; }

// -- Private ---------------------------------------------------------------

void ReconnectionManager::eliminatePlayerUnits(FactionId factionId) {
    auto &units = session_->mutableState().units();
    for (std::size_t i = units.size(); i > 0; --i) {
        std::size_t idx = i - 1;
        if (units[idx] && units[idx]->factionId() == factionId) {
            session_->mutableState().removeUnit(idx);
        }
    }
}

} // namespace game
