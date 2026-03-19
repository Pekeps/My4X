#pragma once

#include "game/GameState.h"

#include <string>

namespace game {

/// Save game state to a binary protobuf file.
/// Creates parent directories if they do not exist.
/// Returns true on success, false on failure.
[[nodiscard]] bool saveGame(const GameState &state, const std::string &filepath);

/// Load game state from a binary protobuf file.
/// Throws std::runtime_error if the file is missing, unreadable, or corrupt.
[[nodiscard]] GameState loadGame(const std::string &filepath);

/// Generate a save-file path with the current timestamp.
/// Format: saves/savegame_YYYYMMDD_HHMMSS.bin
[[nodiscard]] std::string generateSavePath();

/// Return the path to the most recent .bin file in saves/.
/// Throws std::runtime_error if no save files exist.
[[nodiscard]] std::string latestSavePath();

} // namespace game
