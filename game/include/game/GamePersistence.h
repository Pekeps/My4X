#pragma once

#include "game/GameState.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct sqlite3;

namespace game {

/// Summary of a saved game returned by listGames().
struct GameSummary {
    std::string id;
    std::string name;
    int turn = 0;
    int playerCount = 0;
    std::string createdAt;
    std::string updatedAt;
};

/// Persist and retrieve GameState objects via SQLite.
///
/// Serializes the full GameState through protobuf and stores the resulting
/// binary blob alongside lightweight metadata for listing/querying.
class GamePersistence {
  public:
    /// Open (or create) a SQLite database at the given path.
    /// Use ":memory:" for an in-memory database (useful for tests).
    /// Throws std::runtime_error on failure.
    explicit GamePersistence(const std::string &dbPath);

    ~GamePersistence();

    GamePersistence(const GamePersistence &) = delete;
    GamePersistence &operator=(const GamePersistence &) = delete;
    GamePersistence(GamePersistence &&other) noexcept;
    GamePersistence &operator=(GamePersistence &&other) noexcept;

    /// Save a game state under the given id and human-readable name.
    /// If a game with the same id already exists it is overwritten.
    /// Throws std::runtime_error on failure.
    void saveGame(const std::string &gameId, const std::string &gameName, const GameState &state);

    /// Load a previously saved game state by id.
    /// Throws std::runtime_error if the id does not exist or the data is
    /// corrupt.
    [[nodiscard]] GameState loadGame(const std::string &gameId) const;

    /// Return summaries for every saved game, ordered by most-recently-updated
    /// first.
    [[nodiscard]] std::vector<GameSummary> listGames() const;

    /// Delete a saved game by id.  No-op if the id does not exist.
    void deleteGame(const std::string &gameId);

  private:
    /// Ensure the schema exists (idempotent).
    void createSchema();

    /// Count the distinct faction IDs present in the game state.
    [[nodiscard]] static int countPlayers(const GameState &state);

    /// Return the current UTC timestamp as an ISO-8601 string.
    [[nodiscard]] static std::string nowUtc();

    sqlite3 *db_ = nullptr;
};

} // namespace game
