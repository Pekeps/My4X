#pragma once

#include "engine/HexGrid.h"
#include "game/Map.h"

#include <optional>

namespace engine {

// Renders the map as a hex grid using offset coordinates.
// Odd rows are shifted right by half a hex width.
// highlightedTile: if set, that tile is filled with a highlight color.
void drawMap(const game::Map &map, std::optional<hex::TileCoord> highlightedTile = std::nullopt);

} // namespace engine
