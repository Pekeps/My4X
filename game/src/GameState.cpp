#include "game/GameState.h"

namespace game {

void GameState::nextTurn() { ++turn_; }

int GameState::getTurn() const { return turn_; }

} // namespace game
