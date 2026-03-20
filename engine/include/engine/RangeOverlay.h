#pragma once

#include "game/Map.h"
#include "game/Unit.h"

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

} // namespace engine
