#pragma once

#include "game/Unit.h"

#include <memory>
#include <vector>

namespace engine {

// Draws units as 3D markers on the hex grid.
// selectedIndex: index of the selected unit (-1 for none), drawn with a highlight.
void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units, int selectedIndex = -1);

} // namespace engine
