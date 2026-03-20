#include "game/Pathfinding.h"

#include "game/TerrainType.h"
#include "game/Unit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

namespace game {

namespace {

/// Odd-r hex neighbor direction offsets for even rows.
/// Each pair is {row_delta, col_delta}.
constexpr std::array<std::array<int, 2>, NEIGHBOR_DIRECTION_COUNT> EVEN_ROW_DIRECTIONS = {{
    {-1, -1}, // NW
    {-1, 0},  // NE
    {0, -1},  // W
    {0, 1},   // E
    {1, -1},  // SW
    {1, 0},   // SE
}};

/// Odd-r hex neighbor direction offsets for odd rows.
constexpr std::array<std::array<int, 2>, NEIGHBOR_DIRECTION_COUNT> ODD_ROW_DIRECTIONS = {{
    {-1, 0}, // NW
    {-1, 1}, // NE
    {0, -1}, // W
    {0, 1},  // E
    {1, 0},  // SW
    {1, 1},  // SE
}};

/// Hash for TileCoord to use in unordered containers.
struct TileCoordHash {
    std::size_t operator()(const TileCoord &coord) const noexcept {
        auto hash1 = std::hash<int>{}(coord.row);
        auto hash2 = std::hash<int>{}(coord.col);
        return hash1 ^ (hash2 << 1U);
    }
};

/// Node in the A* open set priority queue.
struct OpenNode {
    TileCoord coord;
    int fScore;

    /// Priority queue is a max-heap, so we invert the comparison
    /// to make it a min-heap (lower fScore = higher priority).
    bool operator>(const OpenNode &other) const { return fScore > other.fScore; }
};

/// Sentinel cost value indicating a tile has not been visited.
constexpr int INFINITE_COST = std::numeric_limits<int>::max();

/// Check whether a tile is blocked for the given faction.
/// A tile is blocked if the terrain is impassable or if an enemy unit occupies it.
bool isTileBlocked(int row, int col, const Map &map, const TileRegistry &registry, FactionId factionId) {
    const auto &tile = map.tile(row, col);
    const auto &props = getTerrainProperties(tile.terrainType());

    if (!props.passable) {
        return true;
    }

    // Check for enemy unit occupancy.
    auto units = registry.unitsAt(row, col);
    return std::ranges::any_of(units, [factionId](const auto *unit) { return unit->factionId() != factionId; });
}

} // namespace

int hexDistance(int row1, int col1, int row2, int col2) {
    // Convert odd-r offset coordinates to cube coordinates.
    // For odd-r: x = col - (row - (row & 1)) / 2
    //            z = row
    //            y = -x - z
    int x1 = col1 - ((row1 - (row1 & 1)) / CUBE_COORD_DIVISOR);
    int z1 = row1;
    int y1 = -x1 - z1;

    int x2 = col2 - ((row2 - (row2 & 1)) / CUBE_COORD_DIVISOR);
    int z2 = row2;
    int y2 = -x2 - z2;

    return (std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(z1 - z2)) / CUBE_COORD_DIVISOR;
}

std::vector<TileCoord> hexNeighbors(int row, int col, int mapHeight, int mapWidth) {
    std::vector<TileCoord> neighbors;
    neighbors.reserve(MAX_HEX_NEIGHBORS);

    const auto &directions = ((row & 1) == 0) ? EVEN_ROW_DIRECTIONS : ODD_ROW_DIRECTIONS;

    for (const auto &dir : directions) {
        int nr = row + dir[0];
        int nc = col + dir[1];

        if (nr >= 0 && nr < mapHeight && nc >= 0 && nc < mapWidth) {
            neighbors.push_back({nr, nc});
        }
    }

    return neighbors;
}

std::vector<TileCoord> findPath(int startRow, int startCol, int goalRow, int goalCol, const Map &map,
                                const TileRegistry &registry, FactionId factionId) {
    // Trivial case: start == goal.
    if (startRow == goalRow && startCol == goalCol) {
        return {};
    }

    // Quick check: if the goal tile itself is blocked, no path exists.
    if (isTileBlocked(goalRow, goalCol, map, registry, factionId)) {
        return {};
    }

    int mapHeight = map.height();
    int mapWidth = map.width();

    // A* open set: min-heap ordered by f-score.
    std::priority_queue<OpenNode, std::vector<OpenNode>, std::greater<>> openSet;

    // g-score: best known cost from start to each tile.
    std::unordered_map<TileCoord, int, TileCoordHash> gScore;

    // Track which tile we came from to reconstruct the path.
    std::unordered_map<TileCoord, TileCoord, TileCoordHash> cameFrom;

    TileCoord start = {.row = startRow, .col = startCol};
    TileCoord goal = {.row = goalRow, .col = goalCol};

    gScore[start] = 0;
    int hStart = hexDistance(startRow, startCol, goalRow, goalCol);
    openSet.push({start, hStart});

    while (!openSet.empty()) {
        auto current = openSet.top();
        openSet.pop();

        // Reached the goal — reconstruct the path.
        if (current.coord == goal) {
            std::vector<TileCoord> path;
            TileCoord step = goal;
            while (!(step == start)) {
                path.push_back(step);
                step = cameFrom.at(step);
            }
            // Path was built from goal to start; reverse it.
            std::ranges::reverse(path);
            return path;
        }

        int currentG = gScore[current.coord];

        // Skip stale entries (we may have pushed a node multiple times
        // with different f-scores; only process the one with the best g-score).
        auto it = gScore.find(current.coord);
        if (it != gScore.end() && currentG < it->second) {
            continue;
        }

        auto neighbors = hexNeighbors(current.coord.row, current.coord.col, mapHeight, mapWidth);

        for (const auto &neighbor : neighbors) {
            // Skip blocked tiles.
            if (isTileBlocked(neighbor.row, neighbor.col, map, registry, factionId)) {
                // Exception: the goal tile was already checked above and is passable.
                continue;
            }

            const auto &tile = map.tile(neighbor.row, neighbor.col);
            const auto &props = getTerrainProperties(tile.terrainType());
            int tentativeG = currentG + props.movementCost;

            auto neighborIt = gScore.find(neighbor);
            int bestG = (neighborIt != gScore.end()) ? neighborIt->second : INFINITE_COST;

            if (tentativeG < bestG) {
                gScore[neighbor] = tentativeG;
                cameFrom[neighbor] = current.coord;
                int heuristic = hexDistance(neighbor.row, neighbor.col, goalRow, goalCol);
                openSet.push({neighbor, tentativeG + heuristic});
            }
        }
    }

    // No path found.
    return {};
}

} // namespace game
