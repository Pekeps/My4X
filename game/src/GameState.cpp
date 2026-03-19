#include "game/GameState.h"

#include <algorithm>
#include <stdexcept>

namespace game {

GameState::GameState(int mapRows, int mapCols) : map_(mapRows, mapCols) {}

GameState::GameState(int mapRows, int mapCols, std::uint64_t seed) : map_(mapRows, mapCols, seed) {}

// -- Turn --------------------------------------------------------------

void GameState::nextTurn() { ++turn_; }

int GameState::getTurn() const { return turn_; }

void GameState::setTurn(int turn) { turn_ = turn; }

// -- Map ---------------------------------------------------------------

const Map &GameState::map() const { return map_; }

Map &GameState::mutableMap() { return map_; }

// -- TileRegistry ------------------------------------------------------

const TileRegistry &GameState::registry() const { return registry_; }

TileRegistry &GameState::mutableRegistry() { return registry_; }

// -- Cities ------------------------------------------------------------

CityId GameState::addCity(City city) {
    city.setId(nextCityId_++);
    registry_.registerCity(city.centerRow(), city.centerCol(), city.id());
    cities_.push_back(std::move(city));
    return cities_.back().id();
}

void GameState::removeCity(CityId id) {
    auto iter = std::ranges::find_if(cities_, [id](const City &c) { return c.id() == id; });
    if (iter == cities_.end()) {
        throw std::out_of_range("City not found");
    }
    registry_.unregisterCity(iter->centerRow(), iter->centerCol());
    cities_.erase(iter);
}

const std::vector<City> &GameState::cities() const { return cities_; }

std::vector<City> &GameState::mutableCities() { return cities_; }

std::vector<City> &GameState::cities() { return cities_; }

City *GameState::findCity(CityId id) {
    auto iter = std::ranges::find_if(cities_, [id](const City &c) { return c.id() == id; });
    return iter != cities_.end() ? &*iter : nullptr;
}

void GameState::restoreCity(City city) {
    CityId id = city.id();
    for (const auto &[tRow, tCol] : city.tiles()) {
        registry_.registerCity(tRow, tCol, id);
    }
    cities_.push_back(std::move(city));
}

void GameState::setNextCityId(CityId id) { nextCityId_ = id; }

CityId GameState::nextCityId() const { return nextCityId_; }

// -- Buildings ---------------------------------------------------------

BuildingId GameState::addBuilding(Building building) {
    building.setId(nextBuildingId_++);
    auto const &tiles = building.occupiedTiles();
    if (!tiles.empty()) {
        auto const &first = *tiles.begin();
        registry_.registerBuilding(first.row, first.col, building.id());
    }
    buildings_.push_back(std::move(building));
    return buildings_.back().id();
}

void GameState::removeBuilding(BuildingId id) {
    auto iter = std::ranges::find_if(buildings_, [id](const Building &b) { return b.id() == id; });
    if (iter == buildings_.end()) {
        throw std::out_of_range("Building not found");
    }
    auto const &tiles = iter->occupiedTiles();
    if (!tiles.empty()) {
        auto const &first = *tiles.begin();
        registry_.unregisterBuilding(first.row, first.col);
    }
    buildings_.erase(iter);
}

const std::vector<Building> &GameState::buildings() const { return buildings_; }

void GameState::restoreBuilding(Building building) {
    BuildingId id = building.id();
    for (const auto &coord : building.occupiedTiles()) {
        registry_.registerBuilding(coord.row, coord.col, id);
    }
    buildings_.push_back(std::move(building));
}

void GameState::setNextBuildingId(BuildingId id) { nextBuildingId_ = id; }

BuildingId GameState::nextBuildingId() const { return nextBuildingId_; }

// -- Units -------------------------------------------------------------

std::size_t GameState::addUnit(std::unique_ptr<Unit> unit) {
    registry_.registerUnit(unit->row(), unit->col(), unit.get());
    units_.push_back(std::move(unit));
    return units_.size() - 1;
}

void GameState::removeUnit(std::size_t index) {
    if (index >= units_.size()) {
        throw std::out_of_range("Unit index out of range");
    }
    auto &unit = units_.at(index);
    registry_.unregisterUnit(unit->row(), unit->col(), unit.get());
    units_.erase(units_.begin() + static_cast<std::ptrdiff_t>(index));
}

const std::vector<std::unique_ptr<Unit>> &GameState::units() const { return units_; }

std::vector<std::unique_ptr<Unit>> &GameState::units() { return units_; }

// -- Faction resources -------------------------------------------------

const Resource &GameState::factionResources() const { return factionResources_; }

Resource &GameState::factionResources() { return factionResources_; }

} // namespace game
