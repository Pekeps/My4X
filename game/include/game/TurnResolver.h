#pragma once

#include "game/GameState.h"
#include "game/NeutralAI.h"

#include <vector>

namespace game {

/// Processes end-of-turn logic: collect yields, apply production to build
/// queues, grow city population, reset unit movement, generate neutral AI
/// actions, and advance the turn.
///
/// This is the single point where game state mutates between turns.
/// It is intentionally pure — no rendering, no I/O.
void resolveTurn(GameState &state);

/// Overload that also returns the AI actions generated during the turn.
/// Callers can inspect the returned actions for UI feedback, logging, etc.
void resolveTurn(GameState &state, std::vector<AIAction> &outActions);

} // namespace game
