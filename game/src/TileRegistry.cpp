#include "game/TileRegistry.h"

#include <algorithm>

namespace game {

void TileRegistry::registerCity(int row, int col, CityId city) { cities_[{row, col}] = city; }

void TileRegistry::unregisterCity(int row, int col) { cities_.erase({row, col}); }

void TileRegistry::registerBuilding(int row, int col, BuildingId building) { buildings_[{row, col}] = building; }

void TileRegistry::unregisterBuilding(int row, int col) { buildings_.erase({row, col}); }

void TileRegistry::registerUnit(int row, int col, Unit *unit) { units_[{row, col}].push_back(unit); }

void TileRegistry::unregisterUnit(int row, int col, Unit *unit) {
    auto iter = units_.find({row, col});
    if (iter == units_.end()) {
        return;
    }
    auto &vec = iter->second;
    std::erase(vec, unit);
    if (vec.empty()) {
        units_.erase(iter);
    }
}

std::optional<CityId> TileRegistry::cityAt(int row, int col) const {
    auto iter = cities_.find({row, col});
    if (iter != cities_.end()) {
        return iter->second;
    }
    return std::nullopt;
}

std::optional<BuildingId> TileRegistry::buildingAt(int row, int col) const {
    auto iter = buildings_.find({row, col});
    if (iter != buildings_.end()) {
        return iter->second;
    }
    return std::nullopt;
}

std::vector<Unit *> TileRegistry::unitsAt(int row, int col) const {
    auto iter = units_.find({row, col});
    if (iter != units_.end()) {
        return iter->second;
    }
    return {};
}

bool TileRegistry::isOccupied(int row, int col) const {
    TileCoord coord{.row = row, .col = col};
    return cities_.contains(coord) || buildings_.contains(coord) || units_.contains(coord);
}

} // namespace game
