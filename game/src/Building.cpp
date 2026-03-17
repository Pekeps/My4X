#include "game/Building.h"

#include <utility>

namespace game {

Building::Building(std::string name, Resource yieldPerTurn, Resource constructionCost,
                   std::set<TileCoord> occupiedTiles, std::set<TerrainType> allowedTerrains)
    : name_(std::move(name)), yieldPerTurn_(yieldPerTurn), constructionCost_(constructionCost),
      occupiedTiles_(std::move(occupiedTiles)), allowedTerrains_(std::move(allowedTerrains)) {}

BuildingId Building::id() const { return id_; }
void Building::setId(BuildingId newId) { id_ = newId; }

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

const int FARM_FOOD_YIELD = 2;
const int FARM_PRODUCTION_COST = 20;

const int MINE_PRODUCTION_YIELD = 2;
const int MINE_PRODUCTION_COST = 10;

const int MARKET_GOLD_YIELD = 3;
const int MARKET_PRODUCTION_COST = 30;

Building makeFarm(int row, int col) {
    return Building("Farm", Resource{.gold = 0, .production = 0, .food = FARM_FOOD_YIELD},
                    Resource{.gold = 0, .production = FARM_PRODUCTION_COST, .food = 0}, {{row, col}});
}

Building makeMine(int row, int col) {
    return Building("Mine", Resource{.gold = 0, .production = MINE_PRODUCTION_YIELD, .food = 0},
                    Resource{.gold = 0, .production = MINE_PRODUCTION_COST, .food = 0}, {{row, col}},
                    {TerrainType::Hills});
}

Building makeMarket(int row, int col) {
    return Building("Market", Resource{.gold = MARKET_GOLD_YIELD, .production = 0, .food = 0},
                    Resource{.gold = 0, .production = MARKET_PRODUCTION_COST, .food = 0}, {{row, col}});
}

} // namespace game
