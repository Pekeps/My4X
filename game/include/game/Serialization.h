#pragma once
#include "game/GameState.h"
#include <stdexcept>
#include <string>

namespace game {
// Serialize GameState to binary protobuf bytes.
[[nodiscard]] std::string serializeGameState(const GameState &state);
// Deserialize binary protobuf bytes into a new GameState.
// Throws std::runtime_error if deserialization fails.
[[nodiscard]] GameState deserializeGameState(const std::string &data);
} // namespace game
