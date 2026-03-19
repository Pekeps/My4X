#include "game/TurnResolver.h"

#include "game/Building.h"
#include "game/City.h"
#include "game/Resource.h"

namespace game {

namespace {

/// Sum the resource yields of all buildings whose tiles overlap the city.
Resource collectCityYields(const City &city, const GameState &state) {
    Resource total;
    for (const Building &building : state.buildings()) {
        for (const TileCoord &coord : building.occupiedTiles()) {
            if (city.containsTile(coord.row, coord.col)) {
                total += building.yieldPerTurn();
                break; // count each building once
            }
        }
    }
    return total;
}

} // namespace

void resolveTurn(GameState &state) {
    // 1. Collect yields and apply production / food for each city.
    for (City &city : state.mutableCities()) {
        Resource yields = collectCityYields(city, state);

        // Gold goes to the owning faction's stockpile.
        state.factionResources().gold += yields.gold;

        // Production: base city production + building production yields.
        int totalProduction = City::productionPerTurn() + yields.production;
        auto completed = city.buildQueue().applyProduction(totalProduction);
        if (completed.has_value()) {
            state.addBuilding(std::move(*completed));
        }

        // Food: accumulate surplus, grow population when threshold is met.
        city.addFoodSurplus(yields.food);
        int threshold = City::growthThreshold(city.population());
        if (city.foodSurplus() >= threshold) {
            city.growPopulation(1);
            city.setFoodSurplus(city.foodSurplus() - threshold);
        }
    }

    // 2. Reset unit movement.
    for (auto &unit : state.units()) {
        unit->resetMovement();
    }

    // 3. Advance the turn counter.
    state.nextTurn();
}

} // namespace game
