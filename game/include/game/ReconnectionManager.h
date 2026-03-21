#pragma once

#include "game/GameSession.h"
#include "game/Serialization.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

namespace game {

/// Result of a reconnection attempt.
enum class ReconnectResult : std::uint8_t {
    Success,
    UnknownPlayer,
    AlreadyConnected,
    TimedOut,
};

/// Per-player tracking data managed by ReconnectionManager.
struct PlayerConnectionInfo {
    PlayerId playerId = 0;
    FactionId factionId = 0;
    bool connected = true;

    /// Time point at which the player disconnected. Only meaningful when
    /// connected == false.
    std::chrono::steady_clock::time_point disconnectTime;
};

/// Manages player disconnect/reconnect lifecycle on top of a GameSession.
///
/// Responsibilities:
///  - Track connection status per player
///  - On disconnect: mark player disconnected (units stay on map, don't act)
///  - On reconnect: validate identity, generate a full state snapshot, resume
///  - Timeout: eliminate a player who stays disconnected too long
///
/// This class is pure game logic with no networking dependencies.
class ReconnectionManager {
  public:
    /// Default disconnect timeout (5 minutes).
    static constexpr std::chrono::seconds DEFAULT_DISCONNECT_TIMEOUT{300};

    /// Construct a ReconnectionManager that operates on the given session.
    /// @param session  The game session to manage reconnection for.
    /// @param timeout  How long a player may stay disconnected before
    ///                 elimination. Defaults to 5 minutes.
    explicit ReconnectionManager(GameSession &session,
                                 std::chrono::steady_clock::duration timeout = DEFAULT_DISCONNECT_TIMEOUT);

    // -- Player registration -----------------------------------------------

    /// Register a player for reconnection tracking. Typically called once
    /// per player at session start. Returns false if already registered.
    bool registerPlayer(PlayerId playerId, FactionId factionId);

    /// Unregister a player (e.g., after voluntary leave). Returns false if
    /// the player was not tracked.
    bool unregisterPlayer(PlayerId playerId);

    // -- Disconnect / Reconnect --------------------------------------------

    /// Mark a player as disconnected. Records the disconnect timestamp and
    /// updates the underlying GameSession.
    /// @param now  The current time (injectable for testing).
    /// Returns false if the player is unknown or already disconnected.
    bool handleDisconnect(PlayerId playerId,
                          std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

    /// Attempt to reconnect a player. Validates identity (player must be
    /// known and currently disconnected, and not timed out).
    /// On success, marks the player as connected in GameSession and
    /// generates a full state snapshot via Serialization.
    /// @param now  The current time (injectable for testing).
    ReconnectResult handleReconnect(PlayerId playerId, FactionId factionId,
                                    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

    /// Retrieve the state snapshot produced by the most recent successful
    /// reconnection. Returns nullopt if no snapshot is available.
    [[nodiscard]] const std::optional<std::string> &lastSnapshot() const;

    // -- Timeout management ------------------------------------------------

    /// Check all disconnected players for timeout violations and eliminate
    /// those who have been disconnected for too long.
    /// @param now  The current time (injectable for testing).
    /// Returns the list of player IDs that were eliminated.
    std::vector<PlayerId> checkTimeouts(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

    /// Returns true if the given player has been eliminated due to timeout.
    [[nodiscard]] bool isEliminated(PlayerId playerId) const;

    // -- Query -------------------------------------------------------------

    /// Get the connection info for a player, or nullptr if unknown.
    [[nodiscard]] const PlayerConnectionInfo *getConnectionInfo(PlayerId playerId) const;

    /// Return the number of currently tracked players (not eliminated).
    [[nodiscard]] std::size_t trackedPlayerCount() const;

    /// Return the number of currently connected players.
    [[nodiscard]] std::size_t connectedPlayerCount() const;

    /// Return the number of currently disconnected (but not eliminated)
    /// players.
    [[nodiscard]] std::size_t disconnectedPlayerCount() const;

    /// Return the configured disconnect timeout duration.
    [[nodiscard]] std::chrono::steady_clock::duration disconnectTimeout() const;

  private:
    /// Remove all units belonging to a faction (elimination cleanup).
    void eliminatePlayerUnits(FactionId factionId);

    GameSession *session_;
    std::chrono::steady_clock::duration timeout_;
    std::unordered_map<PlayerId, PlayerConnectionInfo> players_;
    std::unordered_set<PlayerId> eliminated_;
    std::optional<std::string> lastSnapshot_;
};

} // namespace game
