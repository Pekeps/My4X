#pragma once

#include "game/GameState.h"
#include "game/NeutralAI.h"
#include "game/UnitTypeRegistry.h"

#include <vector>

namespace game {

/// Processes end-of-turn logic: collect yields, apply production to build
/// queues, grow city population, reset unit movement, generate neutral AI
/// actions, and advance the turn.
///
/// This is the single point where game state mutates between turns.
/// It is intentionally pure — no rendering, no I/O.
///
/// The overload without a UnitTypeRegistry handles building production only
/// (backward-compatible). Pass a UnitTypeRegistry to also resolve unit
/// production orders.
void resolveTurn(GameState &state);

/// Overload that also returns the AI actions generated during the turn.
/// Callers can inspect the returned actions for UI feedback, logging, etc.
void resolveTurn(GameState &state, std::vector<AIAction> &outActions);

/// Overload that accepts a UnitTypeRegistry for resolving unit production.
/// When a unit production order completes, the unit is spawned adjacent to the
/// city center (or on the city center tile if no adjacent tile is free).
void resolveTurn(GameState &state, const UnitTypeRegistry *unitRegistry);

} // namespace game
