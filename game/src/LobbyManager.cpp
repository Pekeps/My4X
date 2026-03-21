#include "game/LobbyManager.h"

#include <algorithm>

namespace game {

// -- GameSettings helpers ----------------------------------------------------

int GameSettings::mapRows() const {
    switch (mapSize) {
    case MapSize::Small:
        return SMALL_MAP_ROWS;
    case MapSize::Large:
        return LARGE_MAP_ROWS;
    case MapSize::Medium:
    default:
        return MEDIUM_MAP_ROWS;
    }
}

int GameSettings::mapCols() const {
    switch (mapSize) {
    case MapSize::Small:
        return SMALL_MAP_COLS;
    case MapSize::Large:
        return LARGE_MAP_COLS;
    case MapSize::Medium:
    default:
        return MEDIUM_MAP_COLS;
    }
}

// -- LobbyManager ------------------------------------------------------------

GameId LobbyManager::createGame(PlayerId hostPlayerId, const GameSettings &settings) {
    GameId id = nextGameId_++;

    LobbyGame lobby;
    lobby.gameId = id;
    lobby.settings = settings;
    lobby.state = LobbyGameState::WaitingForPlayers;
    lobby.hostPlayerId = hostPlayerId;

    // Pre-allocate player slots.
    lobby.playerSlots.resize(static_cast<std::size_t>(settings.maxPlayers));

    // The host occupies the first slot.
    lobby.playerSlots[0].playerId = hostPlayerId;
    lobby.playerSlots[0].factionId = 1;
    lobby.playerSlots[0].occupied = true;

    games_[id] = std::move(lobby);
    return id;
}

std::vector<GameSummary> LobbyManager::listGames() const {
    std::vector<GameSummary> result;
    result.reserve(games_.size());
    for (const auto &[id, lobby] : games_) {
        result.push_back(makeSummary(lobby));
    }
    return result;
}

std::vector<GameSummary> LobbyManager::listGames(LobbyGameState stateFilter) const {
    std::vector<GameSummary> result;
    for (const auto &[id, lobby] : games_) {
        if (lobby.state == stateFilter) {
            result.push_back(makeSummary(lobby));
        }
    }
    return result;
}

bool LobbyManager::joinGame(GameId gameId, PlayerId playerId) {
    auto iter = games_.find(gameId);
    if (iter == games_.end()) {
        return false;
    }
    auto &lobby = iter->second;

    if (lobby.state != LobbyGameState::WaitingForPlayers) {
        return false;
    }

    // Check if the player is already in the game.
    for (const auto &slot : lobby.playerSlots) {
        if (slot.occupied && slot.playerId == playerId) {
            return false;
        }
    }

    // Find the first empty slot.
    for (auto &slot : lobby.playerSlots) {
        if (!slot.occupied) {
            slot.playerId = playerId;
            slot.factionId = static_cast<FactionId>(occupiedSlotCount(lobby) + 1);
            slot.occupied = true;
            return true;
        }
    }

    // All slots full.
    return false;
}

bool LobbyManager::leaveGame(GameId gameId, PlayerId playerId) {
    auto iter = games_.find(gameId);
    if (iter == games_.end()) {
        return false;
    }
    auto &lobby = iter->second;

    // Can only leave a WaitingForPlayers game.
    if (lobby.state != LobbyGameState::WaitingForPlayers) {
        return false;
    }

    // If the host leaves, remove the entire game.
    if (playerId == lobby.hostPlayerId) {
        games_.erase(iter);
        return true;
    }

    // Find and free the player's slot.
    for (auto &slot : lobby.playerSlots) {
        if (slot.occupied && slot.playerId == playerId) {
            slot.occupied = false;
            slot.playerId = 0;
            slot.factionId = 0;
            return true;
        }
    }

    // Player not found in the game.
    return false;
}

std::optional<GameInfo> LobbyManager::getGameInfo(GameId gameId) const {
    auto iter = games_.find(gameId);
    if (iter == games_.end()) {
        return std::nullopt;
    }
    return makeInfo(iter->second);
}

GameSession *LobbyManager::startGame(GameId gameId, PlayerId requestingPlayerId) {
    auto iter = games_.find(gameId);
    if (iter == games_.end()) {
        return nullptr;
    }
    auto &lobby = iter->second;

    // Only the host can start the game.
    if (requestingPlayerId != lobby.hostPlayerId) {
        return nullptr;
    }

    // Game must be waiting for players.
    if (lobby.state != LobbyGameState::WaitingForPlayers) {
        return nullptr;
    }

    // Must meet minimum player count.
    int players = occupiedSlotCount(lobby);
    if (players < lobby.settings.minPlayers) {
        return nullptr;
    }

    // Create the GameSession.
    int rows = lobby.settings.mapRows();
    int cols = lobby.settings.mapCols();
    auto session = std::make_unique<GameSession>(rows, cols);

    // Register each player in the session.
    FactionId nextFaction = 1;
    for (const auto &slot : lobby.playerSlots) {
        if (slot.occupied) {
            session->addPlayer(slot.playerId, nextFaction);
            nextFaction++;
        }
    }

    lobby.session = std::move(session);
    lobby.state = LobbyGameState::InProgress;

    return lobby.session.get();
}

bool LobbyManager::finishGame(GameId gameId) {
    auto iter = games_.find(gameId);
    if (iter == games_.end()) {
        return false;
    }
    if (iter->second.state != LobbyGameState::InProgress) {
        return false;
    }
    iter->second.state = LobbyGameState::Finished;
    return true;
}

GameSession *LobbyManager::getSession(GameId gameId) {
    auto iter = games_.find(gameId);
    if (iter == games_.end() || iter->second.state != LobbyGameState::InProgress) {
        return nullptr;
    }
    return iter->second.session.get();
}

const GameSession *LobbyManager::getSession(GameId gameId) const {
    auto iter = games_.find(gameId);
    if (iter == games_.end() || iter->second.state != LobbyGameState::InProgress) {
        return nullptr;
    }
    return iter->second.session.get();
}

std::size_t LobbyManager::gameCount() const { return games_.size(); }

bool LobbyManager::hasGame(GameId gameId) const { return games_.contains(gameId); }

// -- Private helpers ---------------------------------------------------------

GameSummary LobbyManager::makeSummary(const LobbyGame &lobby) {
    GameSummary summary;
    summary.gameId = lobby.gameId;
    summary.gameName = lobby.settings.gameName;
    summary.state = lobby.state;
    summary.currentPlayers = occupiedSlotCount(lobby);
    summary.maxPlayers = lobby.settings.maxPlayers;
    return summary;
}

GameInfo LobbyManager::makeInfo(const LobbyGame &lobby) {
    GameInfo info;
    info.gameId = lobby.gameId;
    info.gameName = lobby.settings.gameName;
    info.state = lobby.state;
    info.settings = lobby.settings;
    info.playerSlots = lobby.playerSlots;
    info.hostPlayerId = lobby.hostPlayerId;
    return info;
}

int LobbyManager::occupiedSlotCount(const LobbyGame &lobby) {
    return static_cast<int>(std::count_if(lobby.playerSlots.begin(), lobby.playerSlots.end(),
                                          [](const PlayerSlot &slot) { return slot.occupied; }));
}

} // namespace game
