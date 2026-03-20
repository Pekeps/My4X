#include "game/FogOfWar.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace game {

namespace {

/// Divisor used when converting odd-r offset coordinates to cube coordinates.
constexpr int OFFSET_DIVISOR = 2;

} // namespace

FogOfWar::FogOfWar(int rows, int cols) : rows_(rows), cols_(cols) {}

VisibilityState FogOfWar::getVisibility(FactionId factionId, int row, int col) const {
    const auto *grid = findGrid(factionId);
    if (grid == nullptr) {
        return VisibilityState::Unexplored;
    }
    return (*grid)[index(row, col)];
}

void FogOfWar::recalculate(FactionId factionId, const std::vector<std::unique_ptr<Unit>> &units,
                           const std::vector<City> &cities, const Map &map) {
    auto &grid = ensureGrid(factionId);

    // Phase 1: Demote all Visible tiles to Explored.
    // Unexplored tiles remain Unexplored; Explored tiles stay Explored.
    for (auto &state : grid) {
        if (state == VisibilityState::Visible) {
            state = VisibilityState::Explored;
        }
    }

    // Phase 2: Mark all tiles in sight range of friendly units as Visible.
    for (const auto &unit : units) {
        if (unit->factionId() != factionId) {
            continue;
        }
        applyVision(grid, unit->row(), unit->col(), unit->unitTemplate().sightRange, map);
    }

    // Phase 3: Mark all tiles in sight range of friendly cities as Visible.
    // Vision is computed from each tile the city occupies.
    for (const auto &city : cities) {
        if (std::cmp_not_equal(city.factionId(), factionId)) {
            continue;
        }
        for (const auto &[tileRow, tileCol] : city.tiles()) {
            applyVision(grid, tileRow, tileCol, CITY_SIGHT_RANGE, map);
        }
    }
}

int FogOfWar::rows() const { return rows_; }

int FogOfWar::cols() const { return cols_; }

int FogOfWar::hexDistance(int row1, int col1, int row2, int col2) {
    // Convert odd-r offset coordinates to cube coordinates.
    // For odd-r: x = col - (row - (row & 1)) / 2
    //            z = row
    //            y = -x - z
    int x1 = col1 - ((row1 - (row1 & 1)) / OFFSET_DIVISOR);
    int z1 = row1;
    int y1 = -x1 - z1;

    int x2 = col2 - ((row2 - (row2 & 1)) / OFFSET_DIVISOR);
    int z2 = row2;
    int y2 = -x2 - z2;

    return (std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(z1 - z2)) / OFFSET_DIVISOR;
}

std::vector<VisibilityState> &FogOfWar::ensureGrid(FactionId factionId) {
    auto it = grids_.find(factionId);
    if (it != grids_.end()) {
        return it->second;
    }
    auto [inserted, _] = grids_.emplace(
        factionId, std::vector<VisibilityState>(static_cast<std::size_t>(rows_) * static_cast<std::size_t>(cols_),
                                                VisibilityState::Unexplored));
    return inserted->second;
}

const std::vector<VisibilityState> *FogOfWar::findGrid(FactionId factionId) const {
    auto it = grids_.find(factionId);
    if (it == grids_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::size_t FogOfWar::index(int row, int col) const {
    return (static_cast<std::size_t>(row) * static_cast<std::size_t>(cols_)) + static_cast<std::size_t>(col);
}

void FogOfWar::applyVision(std::vector<VisibilityState> &grid, int centerRow, int centerCol, int sightRange,
                           const Map &map) const {
    // Scan all tiles within a bounding box around the center,
    // then filter by hex distance. This avoids iterating the entire map.
    int minRow = std::max(0, centerRow - sightRange);
    int maxRow = std::min(map.height() - 1, centerRow + sightRange);
    int minCol = std::max(0, centerCol - sightRange);
    int maxCol = std::min(map.width() - 1, centerCol + sightRange);

    for (int row = minRow; row <= maxRow; ++row) {
        for (int col = minCol; col <= maxCol; ++col) {
            if (hexDistance(centerRow, centerCol, row, col) <= sightRange) {
                grid[index(row, col)] = VisibilityState::Visible;
            }
        }
    }
}

} // namespace game
