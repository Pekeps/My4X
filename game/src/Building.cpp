#include "game/Building.h"

#include <utility>

namespace game {

Building::Building(std::string name, Resource yieldPerTurn, Resource constructionCost,
                   std::set<TileCoord> occupiedTiles, std::set<TerrainType> allowedTerrains)
    : name_(std::move(name)), yieldPerTurn_(yieldPerTurn), constructionCost_(constructionCost),
      occupiedTiles_(std::move(occupiedTiles)), allowedTerrains_(std::move(allowedTerrains)) {}

const std::string &Building::name() const { return name_; }
const Resource &Building::yieldPerTurn() const { return yieldPerTurn_; }
const Resource &Building::constructionCost() const { return constructionCost_; }
const std::set<TileCoord> &Building::occupiedTiles() const { return occupiedTiles_; }
const std::set<TerrainType> &Building::allowedTerrains() const { return allowedTerrains_; }

bool Building::canBuildOn(TerrainType terrain) const {
    if (allowedTerrains_.empty()) {
        return true;
    }
    return allowedTerrains_.contains(terrain);
}

// ── Factory functions ────────────────────────────────────────────────────

Building makeFarm(int row, int col) {
    return Building("Farm", Resource{.gold = 0, .production = 0, .food = 2},
                    Resource{.gold = 0, .production = 20, .food = 0}, {{row, col}});
}

Building makeMine(int row, int col) {
    return Building("Mine", Resource{.gold = 0, .production = 2, .food = 0},
                    Resource{.gold = 0, .production = 10, .food = 0}, {{row, col}}, {TerrainType::Hills});
}

Building makeMarket(int row, int col) {
    return Building("Market", Resource{.gold = 3, .production = 0, .food = 0},
                    Resource{.gold = 0, .production = 30, .food = 0}, {{row, col}});
}

} // namespace game
