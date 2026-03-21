#include "game/GamePersistence.h"

#include "game/Serialization.h"

#include <sqlite3.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace game {

namespace {

/// RAII wrapper for sqlite3_stmt that finalizes on destruction.
class Statement {
  public:
    Statement(sqlite3 *db, const char *sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("Failed to prepare statement: ") + sqlite3_errmsg(db));
        }
    }
    ~Statement() {
        if (stmt_ != nullptr) {
            sqlite3_finalize(stmt_);
        }
    }

    Statement(const Statement &) = delete;
    Statement &operator=(const Statement &) = delete;
    Statement(Statement &&) = delete;
    Statement &operator=(Statement &&) = delete;

    [[nodiscard]] sqlite3_stmt *get() const { return stmt_; }

  private:
    sqlite3_stmt *stmt_ = nullptr;
};

} // namespace

GamePersistence::GamePersistence(const std::string &dbPath) {
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ != nullptr ? sqlite3_errmsg(db_) : "unknown error";
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("Failed to open SQLite database: " + msg);
    }
    createSchema();
}

GamePersistence::~GamePersistence() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
}

GamePersistence::GamePersistence(GamePersistence &&other) noexcept : db_(other.db_) { other.db_ = nullptr; }

GamePersistence &GamePersistence::operator=(GamePersistence &&other) noexcept {
    if (this != &other) {
        if (db_ != nullptr) {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

void GamePersistence::createSchema() {
    const char *sql = "CREATE TABLE IF NOT EXISTS games ("
                      "  id TEXT PRIMARY KEY,"
                      "  name TEXT NOT NULL,"
                      "  turn INTEGER NOT NULL,"
                      "  player_count INTEGER NOT NULL,"
                      "  created_at TEXT NOT NULL,"
                      "  updated_at TEXT NOT NULL,"
                      "  state_blob BLOB NOT NULL"
                      ");";
    char *errMsg = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string msg = errMsg != nullptr ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to create schema: " + msg);
    }
}

void GamePersistence::saveGame(const std::string &gameId, const std::string &gameName, const GameState &state) {
    std::string blob = serializeGameState(state);
    int players = countPlayers(state);
    std::string now = nowUtc();

    // INSERT OR REPLACE — simple overwrite strategy.
    const char *sql = "INSERT OR REPLACE INTO games "
                      "(id, name, turn, player_count, created_at, updated_at, state_blob) "
                      "VALUES (?1, ?2, ?3, ?4, ?5, ?5, ?6);";
    Statement stmt(db_, sql);

    static constexpr int BIND_ID = 1;
    static constexpr int BIND_NAME = 2;
    static constexpr int BIND_TURN = 3;
    static constexpr int BIND_PLAYER_COUNT = 4;
    static constexpr int BIND_TIMESTAMP = 5;
    static constexpr int BIND_BLOB = 6;

    sqlite3_bind_text(stmt.get(), BIND_ID, gameId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), BIND_NAME, gameName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), BIND_TURN, state.getTurn());
    sqlite3_bind_int(stmt.get(), BIND_PLAYER_COUNT, players);
    sqlite3_bind_text(stmt.get(), BIND_TIMESTAMP, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt.get(), BIND_BLOB, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Failed to save game: ") + sqlite3_errmsg(db_));
    }
}

GameState GamePersistence::loadGame(const std::string &gameId) const {
    const char *sql = "SELECT state_blob FROM games WHERE id = ?1;";
    Statement stmt(db_, sql);

    static constexpr int BIND_ID = 1;
    sqlite3_bind_text(stmt.get(), BIND_ID, gameId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
        throw std::runtime_error("Game not found: " + gameId);
    }

    static constexpr int COL_BLOB = 0;
    const void *blobData = sqlite3_column_blob(stmt.get(), COL_BLOB);
    int blobSize = sqlite3_column_bytes(stmt.get(), COL_BLOB);

    if (blobData == nullptr || blobSize <= 0) {
        throw std::runtime_error("Empty or null blob for game: " + gameId);
    }

    std::string data(static_cast<const char *>(blobData), static_cast<std::string::size_type>(blobSize));
    return deserializeGameState(data);
}

std::vector<GameSummary> GamePersistence::listGames() const {
    static constexpr int COL_ID = 0;
    static constexpr int COL_NAME = 1;
    static constexpr int COL_TURN = 2;
    static constexpr int COL_PLAYER_COUNT = 3;
    static constexpr int COL_CREATED_AT = 4;
    static constexpr int COL_UPDATED_AT = 5;

    std::vector<GameSummary> results;

    auto callback = [](void *data, int argc, char **argv, char ** /*colNames*/) -> int {
        static constexpr int EXPECTED_COLS = 6;
        auto *vec = static_cast<std::vector<GameSummary> *>(data);
        if (argc < EXPECTED_COLS) {
            return 0;
        }
        auto &summary = vec->emplace_back();
        if (argv[COL_ID] != nullptr) {
            summary.id = argv[COL_ID];
        }
        if (argv[COL_NAME] != nullptr) {
            summary.name = argv[COL_NAME];
        }
        if (argv[COL_TURN] != nullptr) {
            summary.turn = std::stoi(argv[COL_TURN]);
        }
        if (argv[COL_PLAYER_COUNT] != nullptr) {
            summary.playerCount = std::stoi(argv[COL_PLAYER_COUNT]);
        }
        if (argv[COL_CREATED_AT] != nullptr) {
            summary.createdAt = argv[COL_CREATED_AT];
        }
        if (argv[COL_UPDATED_AT] != nullptr) {
            summary.updatedAt = argv[COL_UPDATED_AT];
        }
        return 0;
    };

    char *errMsg = nullptr;
    const char *sql = "SELECT id, name, turn, player_count, created_at, updated_at "
                      "FROM games ORDER BY updated_at DESC;";
    sqlite3_exec(db_, sql, callback, &results, &errMsg);
    if (errMsg != nullptr) {
        sqlite3_free(errMsg);
    }

    return results;
}

void GamePersistence::deleteGame(const std::string &gameId) {
    const char *sql = "DELETE FROM games WHERE id = ?1;";
    Statement stmt(db_, sql);

    static constexpr int BIND_ID = 1;
    sqlite3_bind_text(stmt.get(), BIND_ID, gameId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Failed to delete game: ") + sqlite3_errmsg(db_));
    }
}

int GamePersistence::countPlayers(const GameState &state) {
    // Count distinct faction IDs across all units and cities.
    std::set<FactionId> factionIds;
    for (const auto &unit : state.units()) {
        factionIds.insert(unit->factionId());
    }
    for (const auto &city : state.cities()) {
        factionIds.insert(static_cast<FactionId>(city.factionId()));
    }
    return static_cast<int>(factionIds.size());
}

std::string GamePersistence::nowUtc() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace game
