#include "game/GameSession.h"

#include "game/Unit.h"

#include <algorithm>
#include <variant>

namespace game {

GameSession::GameSession(int mapRows, int mapCols) : state_(mapRows, mapCols) {}

// -- Player management ---------------------------------------------------

bool GameSession::addPlayer(PlayerId playerId, FactionId factionId) {
    if (players_.contains(playerId)) {
        return false;
    }
    PlayerInfo info;
    info.playerId = playerId;
    info.factionId = factionId;
    info.status = PlayerStatus::Connected;
    info.endedTurn = false;
    players_[playerId] = info;
    return true;
}

bool GameSession::removePlayer(PlayerId playerId) { return players_.erase(playerId) > 0; }

bool GameSession::connectPlayer(PlayerId playerId) {
    auto iter = players_.find(playerId);
    if (iter == players_.end()) {
        return false;
    }
    iter->second.status = PlayerStatus::Connected;
    return true;
}

bool GameSession::disconnectPlayer(PlayerId playerId) {
    auto iter = players_.find(playerId);
    if (iter == players_.end()) {
        return false;
    }
    iter->second.status = PlayerStatus::Disconnected;
    return true;
}

FactionId GameSession::getPlayerFaction(PlayerId playerId) const {
    auto iter = players_.find(playerId);
    if (iter == players_.end()) {
        return 0;
    }
    return iter->second.factionId;
}

const PlayerInfo *GameSession::getPlayerInfo(PlayerId playerId) const {
    auto iter = players_.find(playerId);
    if (iter == players_.end()) {
        return nullptr;
    }
    return &iter->second;
}

std::size_t GameSession::playerCount() const { return players_.size(); }

bool GameSession::hasPlayer(PlayerId playerId) const { return players_.contains(playerId); }

// -- Action processing ---------------------------------------------------

bool GameSession::unitBelongsToPlayer(PlayerId playerId, std::size_t unitIndex) const {
    FactionId faction = getPlayerFaction(playerId);
    if (faction == 0) {
        return false;
    }
    const auto &units = state_.units();
    if (unitIndex >= units.size()) {
        return false;
    }
    return units[unitIndex]->factionId() == faction;
}

ActionResult GameSession::applyAction(PlayerId playerId, const GameAction &action) {
    if (!hasPlayer(playerId)) {
        return ActionResult::InvalidPlayer;
    }

    // Action classes validate their own preconditions (unit exists, is alive,
    // has movement, etc.). We verify player existence above; delegate the
    // rest to the concrete action's execute method.
    bool success = std::visit(
        [this](const auto &act) -> bool {
            auto result = act.execute(state_);
            return result.executed;
        },
        action);

    return success ? ActionResult::Success : ActionResult::ActionFailed;
}

// -- Turn management -----------------------------------------------------

bool GameSession::endTurn(PlayerId playerId) {
    auto iter = players_.find(playerId);
    if (iter == players_.end()) {
        return false;
    }
    iter->second.endedTurn = true;
    return true;
}

bool GameSession::allPlayersEndedTurn() const {
    if (players_.empty()) {
        return false;
    }
    return std::ranges::all_of(players_, [](const auto &entry) {
        const auto &info = entry.second;
        // Disconnected players are skipped; connected players must have ended their turn.
        return info.status == PlayerStatus::Disconnected || info.endedTurn;
    });
}

bool GameSession::tryAdvanceTurn() {
    if (!allPlayersEndedTurn()) {
        return false;
    }
    resolveTurn(state_);
    resetTurnFlags();
    return true;
}

int GameSession::currentTurn() const { return state_.getTurn(); }

// -- State access --------------------------------------------------------

const GameState &GameSession::getStateSnapshot() const { return state_; }

GameState &GameSession::mutableState() { return state_; }

// -- Private helpers -----------------------------------------------------

void GameSession::resetTurnFlags() {
    for (auto &[id, info] : players_) {
        info.endedTurn = false;
    }
}

} // namespace game
