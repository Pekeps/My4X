#pragma once

#include "game/Faction.h"
#include "game/Map.h"
#include "game/TileCoord.h"
#include "game/TileRegistry.h"

#include <vector>

namespace game {

/// Maximum number of hex neighbors any tile can have.
static constexpr int MAX_HEX_NEIGHBORS = 6;

/// Divisor for converting odd-r offset coordinates to cube coordinates.
static constexpr int CUBE_COORD_DIVISOR = 2;

/// Number of neighbor direction pairs for even rows in odd-r hex grid.
static constexpr int NEIGHBOR_DIRECTION_COUNT = 6;

/// Compute hex distance between two tiles using odd-r offset coordinates.
/// Converts to cube coordinates and returns the Chebyshev distance.
[[nodiscard]] int hexDistance(int row1, int col1, int row2, int col2);

/// Return the valid hex neighbors of the given tile, respecting map bounds.
/// Uses odd-r offset coordinate system (odd rows are shifted right).
[[nodiscard]] std::vector<TileCoord> hexNeighbors(int row, int col, int mapHeight, int mapWidth);

/// Find the optimal path from start to goal on a hex grid using A*.
///
/// The returned path excludes the start tile and includes the goal tile.
/// Returns an empty vector if no path exists (goal unreachable).
///
/// @param startRow    Starting tile row.
/// @param startCol    Starting tile column.
/// @param goalRow     Destination tile row.
/// @param goalCol     Destination tile column.
/// @param map         The game map (for terrain lookups).
/// @param registry    Tile registry (for unit occupancy checks).
/// @param factionId   Faction of the moving unit (enemy units block tiles).
[[nodiscard]] std::vector<TileCoord> findPath(int startRow, int startCol, int goalRow, int goalCol, const Map &map,
                                              const TileRegistry &registry, FactionId factionId);

} // namespace game
