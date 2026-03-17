#pragma once

#include "game/Unit.h"

#include <memory>
#include <vector>

namespace engine {

// Draws units as 3D markers on the hex grid.
// Units are placed at the center of their hex tile, raised slightly above the ground.
void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units);

} // namespace engine
