#pragma once

#include "game/Map.h"
#include "game/Pathfinding.h"
#include "game/Unit.h"

#include <vector>

namespace engine {

/// Draws an attack range overlay for ranged units, highlighting all tiles
/// within attack range using a distinct color (different from movement range).
///
/// Only draws the overlay if the unit has attackRange > 1 (ranged unit).
/// The overlay is drawn as filled hex tiles in a semi-transparent red/orange.
///
/// @param unit The selected unit to show attack range for.
/// @param map The game map (for bounds checking).
void drawAttackRangeOverlay(const game::Unit &unit, const game::Map &map);

/// Draws a movement range overlay for the selected unit, highlighting all
/// tiles reachable with remaining movement points.
///
/// The overlay is drawn as filled hex tiles in a semi-transparent blue.
///
/// @param reachableTiles Pre-computed set of reachable tiles with costs.
void drawMovementRangeOverlay(const std::vector<game::ReachableTile> &reachableTiles);

} // namespace engine
