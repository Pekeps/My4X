#include "game/City.h"

#include <cassert>
#include <utility>

namespace game {

City::City(std::string name, int centerRow, int centerCol, int factionId)
    : name_(std::move(name)), centerRow_(centerRow), centerCol_(centerCol), factionId_(factionId) {
    tiles_.emplace(centerRow_, centerCol_);
}

CityId City::id() const { return id_; }
void City::setId(CityId newId) { id_ = newId; }

const std::string &City::name() const { return name_; }
int City::centerRow() const { return centerRow_; }
int City::centerCol() const { return centerCol_; }
int City::factionId() const { return factionId_; }
int City::population() const { return population_; }

void City::growPopulation(int amount) {
    assert(amount >= 0);
    population_ += amount;
}

bool City::addTile(int row, int col) {
    auto [iter, inserted] = tiles_.emplace(row, col);
    return inserted;
}

const std::set<std::pair<int, int>> &City::tiles() const { return tiles_; }

bool City::containsTile(int row, int col) const { return tiles_.contains({row, col}); }

int City::foodSurplus() const { return foodSurplus_; }
void City::setFoodSurplus(int surplus) { foodSurplus_ = surplus; }
void City::addFoodSurplus(int amount) { foodSurplus_ += amount; }

BuildQueue &City::buildQueue() { return buildQueue_; }
const BuildQueue &City::buildQueue() const { return buildQueue_; }

int City::growthThreshold(int population) { return 10 + 5 * population; }

int City::productionPerTurn() {
    static constexpr int kBaseProductionPerTurn = 5;
    return kBaseProductionPerTurn;
}

} // namespace game
