#pragma once

#include "game/AttackAction.h"
#include "game/CaptureAction.h"
#include "game/Faction.h"
#include "game/GameState.h"
#include "game/MoveAction.h"
#include "game/TurnResolver.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace game {

/// Unique identifier for a player in a multiplayer session.
using PlayerId = std::uint32_t;

/// A game action that a player can submit to the session.
/// Wraps all concrete action types (move, attack, capture) in a variant.
using GameAction = std::variant<MoveAction, AttackAction, CaptureAction>;

/// Result of applying a game action.
enum class ActionResult : std::uint8_t {
    Success,
    InvalidPlayer,
    NotPlayersTurn,
    ActionFailed,
};

/// Connection status of a player.
enum class PlayerStatus : std::uint8_t {
    Connected,
    Disconnected,
};

/// Information about a player in the session.
struct PlayerInfo {
    PlayerId playerId = 0;
    FactionId factionId = 0;
    PlayerStatus status = PlayerStatus::Connected;
    bool endedTurn = false;
};

/// Manages a multiplayer game session, wrapping GameState with player
/// management, turn tracking, and action processing.
///
/// This class is pure game logic with no networking dependencies, making
/// it fully testable in isolation.
class GameSession {
  public:
    /// Default map dimensions for a new session.
    static constexpr int DEFAULT_MAP_ROWS = 20;
    static constexpr int DEFAULT_MAP_COLS = 20;

    /// Construct a session with the given map dimensions.
    explicit GameSession(int mapRows = DEFAULT_MAP_ROWS, int mapCols = DEFAULT_MAP_COLS);

    // -- Player management -----------------------------------------------

    /// Add a player to the session with the given faction assignment.
    /// Returns false if the player is already registered.
    bool addPlayer(PlayerId playerId, FactionId factionId);

    /// Remove a player from the session.
    /// Returns false if the player was not found.
    bool removePlayer(PlayerId playerId);

    /// Mark a player as connected.
    /// Returns false if the player was not found.
    bool connectPlayer(PlayerId playerId);

    /// Mark a player as disconnected.
    /// Returns false if the player was not found.
    bool disconnectPlayer(PlayerId playerId);

    /// Get the faction assigned to a player.
    /// Returns 0 if the player is not found.
    [[nodiscard]] FactionId getPlayerFaction(PlayerId playerId) const;

    /// Get the status of a player.
    /// Returns nullopt if the player is not found.
    [[nodiscard]] const PlayerInfo *getPlayerInfo(PlayerId playerId) const;

    /// Return the number of registered players.
    [[nodiscard]] std::size_t playerCount() const;

    /// Check whether a player is registered.
    [[nodiscard]] bool hasPlayer(PlayerId playerId) const;

    /// Return all registered player IDs.
    [[nodiscard]] std::vector<PlayerId> playerIds() const;

    // -- Action processing -----------------------------------------------

    /// Validate and apply a game action submitted by a player.
    /// Checks that the player exists and the action's unit belongs to
    /// the player's faction before delegating to the concrete action.
    ActionResult applyAction(PlayerId playerId, const GameAction &action);

    // -- Turn management -------------------------------------------------

    /// Mark a player as having ended their turn.
    /// Returns false if the player was not found.
    bool endTurn(PlayerId playerId);

    /// Check whether all connected players have ended their turn.
    [[nodiscard]] bool allPlayersEndedTurn() const;

    /// Attempt to advance the turn. Resolves the turn if all connected
    /// players have ended theirs. Returns true if the turn was advanced.
    bool tryAdvanceTurn();

    /// Get the current turn number.
    [[nodiscard]] int currentTurn() const;

    // -- State access ----------------------------------------------------

    /// Get the current game state snapshot (const reference).
    [[nodiscard]] const GameState &getStateSnapshot() const;

    /// Get mutable access to the game state (for setup/testing).
    [[nodiscard]] GameState &mutableState();

  private:
    /// Check whether a unit at the given index belongs to the player's faction.
    [[nodiscard]] bool unitBelongsToPlayer(PlayerId playerId, std::size_t unitIndex) const;

    /// Reset all players' endedTurn flags for the next turn.
    void resetTurnFlags();

    GameState state_;
    std::unordered_map<PlayerId, PlayerInfo> players_;
};

} // namespace game
