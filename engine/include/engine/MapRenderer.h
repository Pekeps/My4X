#pragma once

#include "engine/HexGrid.h"
#include "game/FogOfWar.h"
#include "game/Map.h"

#include <optional>

namespace engine {

// Renders the map as a hex grid using offset coordinates.
// Odd rows are shifted right by half a hex width.
// highlightedTile: if set, that tile is filled with a highlight color.
// fog / playerFactionId: when fog is non-null, unexplored tiles are drawn
// black and explored tiles are dimmed.
void drawMap(const game::Map &map, std::optional<hex::TileCoord> highlightedTile = std::nullopt,
             const game::FogOfWar *fog = nullptr, game::FactionId playerFactionId = 0);

} // namespace engine
