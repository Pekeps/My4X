#pragma once

#include "game/GameSession.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace game {

/// Unique identifier for a game lobby.
using GameId = std::uint32_t;

/// Possible states of a lobby game.
enum class LobbyGameState : std::uint8_t {
    WaitingForPlayers,
    InProgress,
    Finished,
};

/// Map size presets for game creation.
enum class MapSize : std::uint8_t {
    Small,
    Medium,
    Large,
};

/// Configuration settings for a new game.
struct GameSettings {
    MapSize mapSize = MapSize::Medium;
    int maxPlayers = DEFAULT_MAX_PLAYERS;
    int minPlayers = DEFAULT_MIN_PLAYERS;
    /// Turn deadline in seconds; 0 means no deadline.
    int turnDeadlineSeconds = 0;
    /// Random seed for map generation; 0 means use a generated seed.
    std::uint64_t randomSeed = 0;
    std::string gameName = "New Game";

    /// Default player count limits.
    static constexpr int DEFAULT_MIN_PLAYERS = 2;
    static constexpr int DEFAULT_MAX_PLAYERS = 8;

    /// Map dimension constants for each size preset.
    static constexpr int SMALL_MAP_ROWS = 16;
    static constexpr int SMALL_MAP_COLS = 16;
    static constexpr int MEDIUM_MAP_ROWS = 32;
    static constexpr int MEDIUM_MAP_COLS = 32;
    static constexpr int LARGE_MAP_ROWS = 64;
    static constexpr int LARGE_MAP_COLS = 64;

    /// Return the map row count for the configured map size.
    [[nodiscard]] int mapRows() const;

    /// Return the map column count for the configured map size.
    [[nodiscard]] int mapCols() const;
};

/// Information about a player slot in a lobby game.
struct PlayerSlot {
    PlayerId playerId = 0;
    FactionId factionId = 0;
    bool occupied = false;
};

/// Summary information for listing available games.
struct GameSummary {
    GameId gameId = 0;
    std::string gameName;
    LobbyGameState state = LobbyGameState::WaitingForPlayers;
    int currentPlayers = 0;
    int maxPlayers = 0;
};

/// Detailed information about a single lobby game.
struct GameInfo {
    GameId gameId = 0;
    std::string gameName;
    LobbyGameState state = LobbyGameState::WaitingForPlayers;
    GameSettings settings;
    std::vector<PlayerSlot> playerSlots;
    PlayerId hostPlayerId = 0;
};

/// Manages pre-game lobbies: creation, joining, configuration, and game start.
///
/// This class is pure game logic with no networking or rendering
/// dependencies, making it fully testable in isolation.
class LobbyManager {
  public:
    /// Starting game ID for newly created games.
    static constexpr GameId INITIAL_GAME_ID = 1;

    /// Create a new game lobby with the given settings.
    /// The creating player becomes the host. Returns the new game's ID.
    GameId createGame(PlayerId hostPlayerId, const GameSettings &settings);

    /// Return summaries of all games (any state).
    [[nodiscard]] std::vector<GameSummary> listGames() const;

    /// Return summaries of games in a specific state.
    [[nodiscard]] std::vector<GameSummary> listGames(LobbyGameState stateFilter) const;

    /// A player joins a game lobby. Returns true on success.
    /// Fails if the game does not exist, is not in WaitingForPlayers state,
    /// the player is already in the game, or the game is full.
    bool joinGame(GameId gameId, PlayerId playerId);

    /// A player leaves a game lobby. Returns true on success.
    /// If the host leaves a WaitingForPlayers game, the game is removed.
    /// If a non-host player leaves, their slot is freed.
    bool leaveGame(GameId gameId, PlayerId playerId);

    /// Get detailed information about a game. Returns nullopt if not found.
    [[nodiscard]] std::optional<GameInfo> getGameInfo(GameId gameId) const;

    /// Start a game, transitioning it from WaitingForPlayers to InProgress.
    /// Only the host can start, and minPlayers must be met.
    /// Returns a pointer to the created GameSession on success, nullptr on failure.
    GameSession *startGame(GameId gameId, PlayerId requestingPlayerId);

    /// Mark a game as finished. Returns true if the game was in-progress.
    bool finishGame(GameId gameId);

    /// Get the GameSession for an in-progress game. Returns nullptr if not found
    /// or not in InProgress state.
    [[nodiscard]] GameSession *getSession(GameId gameId);
    [[nodiscard]] const GameSession *getSession(GameId gameId) const;

    /// Return the total number of tracked games (any state).
    [[nodiscard]] std::size_t gameCount() const;

    /// Check whether a game exists.
    [[nodiscard]] bool hasGame(GameId gameId) const;

  private:
    /// Internal representation of a lobby game.
    struct LobbyGame {
        GameId gameId = 0;
        GameSettings settings;
        LobbyGameState state = LobbyGameState::WaitingForPlayers;
        PlayerId hostPlayerId = 0;
        std::vector<PlayerSlot> playerSlots;
        /// Created when the game starts.
        std::unique_ptr<GameSession> session;
    };

    /// Build a GameSummary from internal state.
    [[nodiscard]] static GameSummary makeSummary(const LobbyGame &lobby);

    /// Build a GameInfo from internal state.
    [[nodiscard]] static GameInfo makeInfo(const LobbyGame &lobby);

    /// Count the number of occupied player slots.
    [[nodiscard]] static int occupiedSlotCount(const LobbyGame &lobby);

    GameId nextGameId_ = INITIAL_GAME_ID;
    std::unordered_map<GameId, LobbyGame> games_;
};

} // namespace game
