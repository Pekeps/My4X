#pragma once

#include "game/Map.h"

namespace engine {

// Renders the map as a hex grid using offset coordinates.
// Odd rows are shifted right by half a hex width.
void drawMap(const game::Map &map);

} // namespace engine
