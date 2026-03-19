#pragma once

#include "game/GameState.h"

namespace game {

/// Processes end-of-turn logic: collect yields, apply production to build
/// queues, grow city population, reset unit movement, and advance the turn.
///
/// This is the single point where game state mutates between turns.
/// It is intentionally pure — no rendering, no I/O.
void resolveTurn(GameState &state);

} // namespace game
