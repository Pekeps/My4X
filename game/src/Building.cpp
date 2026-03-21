#include "game/Building.h"

#include <utility>

namespace game {

Building::Building(std::string name, Resource yieldPerTurn, Resource constructionCost,
                   std::set<TileCoord> occupiedTiles, std::set<TerrainType> allowedTerrains, std::string modelKey,
                   FactionId factionId, bool underConstruction)
    : name_(std::move(name)), yieldPerTurn_(yieldPerTurn), constructionCost_(constructionCost),
      occupiedTiles_(std::move(occupiedTiles)), allowedTerrains_(std::move(allowedTerrains)),
      modelKey_(std::move(modelKey)), factionId_(factionId), underConstruction_(underConstruction) {}

BuildingId Building::id() const { return id_; }
void Building::setId(BuildingId newId) { id_ = newId; }

const std::string &Building::name() const { return name_; }
const Resource &Building::yieldPerTurn() const { return yieldPerTurn_; }
const Resource &Building::constructionCost() const { return constructionCost_; }
const std::set<TileCoord> &Building::occupiedTiles() const { return occupiedTiles_; }
const std::set<TerrainType> &Building::allowedTerrains() const { return allowedTerrains_; }
const std::string &Building::modelKey() const { return modelKey_; }

FactionId Building::factionId() const { return factionId_; }
void Building::setFactionId(FactionId fid) { factionId_ = fid; }

bool Building::underConstruction() const { return underConstruction_; }
void Building::setUnderConstruction(bool value) { underConstruction_ = value; }

bool Building::canBuildOn(TerrainType terrain) const {
    if (allowedTerrains_.empty()) {
        return true;
    }
    return allowedTerrains_.contains(terrain);
}

// ── Factory functions ────────────────────────────────────────────────────

static constexpr int CITY_CENTER_PRODUCTION_YIELD = 1;
static constexpr int CITY_CENTER_FOOD_YIELD = 1;
static constexpr int CITY_CENTER_PRODUCTION_COST = 0;

static constexpr int FARM_FOOD_YIELD = 2;
static constexpr int FARM_PRODUCTION_COST = 20;

static constexpr int MINE_PRODUCTION_YIELD = 2;
static constexpr int MINE_PRODUCTION_COST = 10;

static constexpr int LUMBER_MILL_PRODUCTION_YIELD = 2;
static constexpr int LUMBER_MILL_PRODUCTION_COST = 15;

static constexpr int BARRACKS_PRODUCTION_COST = 25;

static constexpr int MARKET_GOLD_YIELD = 3;
static constexpr int MARKET_PRODUCTION_COST = 30;

Building makeCityCenter(int row, int col) {
    return Building("City Center",
                    Resource{.gold = 0, .production = CITY_CENTER_PRODUCTION_YIELD, .food = CITY_CENTER_FOOD_YIELD},
                    Resource{.gold = 0, .production = CITY_CENTER_PRODUCTION_COST, .food = 0}, {{row, col}}, {},
                    "building_city_center");
}

Building makeFarm(int row, int col) {
    return Building("Farm", Resource{.gold = 0, .production = 0, .food = FARM_FOOD_YIELD},
                    Resource{.gold = 0, .production = FARM_PRODUCTION_COST, .food = 0}, {{row, col}}, {},
                    "building_farm");
}

Building makeMine(int row, int col) {
    return Building("Mine", Resource{.gold = 0, .production = MINE_PRODUCTION_YIELD, .food = 0},
                    Resource{.gold = 0, .production = MINE_PRODUCTION_COST, .food = 0}, {{row, col}},
                    {TerrainType::Hills}, "building_mine");
}

Building makeLumberMill(int row, int col) {
    return Building("Lumber Mill", Resource{.gold = 0, .production = LUMBER_MILL_PRODUCTION_YIELD, .food = 0},
                    Resource{.gold = 0, .production = LUMBER_MILL_PRODUCTION_COST, .food = 0}, {{row, col}},
                    {TerrainType::Forest}, "building_lumber_mill");
}

Building makeBarracks(int row, int col) {
    return Building("Barracks", Resource{.gold = 0, .production = 0, .food = 0},
                    Resource{.gold = 0, .production = BARRACKS_PRODUCTION_COST, .food = 0}, {{row, col}}, {},
                    "building_barracks");
}

Building makeMarket(int row, int col) {
    return Building("Market", Resource{.gold = MARKET_GOLD_YIELD, .production = 0, .food = 0},
                    Resource{.gold = 0, .production = MARKET_PRODUCTION_COST, .food = 0}, {{row, col}}, {},
                    "building_market");
}

} // namespace game
