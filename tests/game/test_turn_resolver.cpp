#include "game/TurnResolver.h"

#include "game/Building.h"
#include "game/City.h"
#include "game/GameState.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>
#include <memory>

using namespace game;

// ---------------------------------------------------------------------------
// Helper: create a small GameState with a city that owns a specific tile set.
// ---------------------------------------------------------------------------
static GameState makeTestState() {
    // 10x10 map is enough for all tests.
    return GameState(10, 10);
}

// ---------------------------------------------------------------------------
// Turn counter advancement
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, AdvancesTurnCounter) {
    auto state = makeTestState();
    EXPECT_EQ(state.getTurn(), 1);
    resolveTurn(state);
    EXPECT_EQ(state.getTurn(), 2);
    resolveTurn(state);
    EXPECT_EQ(state.getTurn(), 3);
}

// ---------------------------------------------------------------------------
// Unit movement reset
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, ResetsUnitMovement) {
    auto state = makeTestState();
    auto warrior = std::make_unique<Warrior>(0, 0);
    // Consume some movement.
    warrior->moveTo(0, 1);
    EXPECT_LT(warrior->movementRemaining(), warrior->movement());
    state.addUnit(std::move(warrior));

    resolveTurn(state);

    EXPECT_EQ(state.units()[0]->movementRemaining(), state.units()[0]->movement());
}

// ---------------------------------------------------------------------------
// Gold yield collection
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, CollectsGoldFromBuildingsToFaction) {
    auto state = makeTestState();

    // Add a city at (1,1).
    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    // Place a Market on tile (2,2) which belongs to the city.
    state.addBuilding(makeMarket(2, 2));

    EXPECT_EQ(state.factionResources().gold, 0);

    resolveTurn(state);

    // Market yields 3 gold.
    EXPECT_EQ(state.factionResources().gold, 3);
}

TEST(TurnResolverTest, GoldAccumulatesAcrossTurns) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    state.addBuilding(makeMarket(2, 2));

    resolveTurn(state);
    resolveTurn(state);

    EXPECT_EQ(state.factionResources().gold, 6);
}

// ---------------------------------------------------------------------------
// Production applied to build queue
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, AppliesProductionToBuildQueue) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    // Enqueue a Mine (costs 10 production). Base production is 5.
    state.mutableCities()[0].buildQueue().enqueue(makeMine, 3, 3);

    resolveTurn(state);

    // After 1 turn: 5 base production applied. 10-5=5 remaining.
    EXPECT_EQ(state.mutableCities()[0].buildQueue().accumulatedProduction(), 5);
    EXPECT_FALSE(state.mutableCities()[0].buildQueue().isEmpty());
}

TEST(TurnResolverTest, BuildingCompletesAndGetsPlaced) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    // Mine costs 10. Base production = 5. Need 2 turns.
    state.mutableCities()[0].buildQueue().enqueue(makeMine, 3, 3);

    resolveTurn(state); // 5 accumulated
    EXPECT_FALSE(state.mutableCities()[0].buildQueue().isEmpty());

    resolveTurn(state); // 10 accumulated -> completes
    EXPECT_TRUE(state.mutableCities()[0].buildQueue().isEmpty());

    // The mine should be registered as a building in the state.
    bool found = false;
    for (const Building &b : state.buildings()) {
        if (b.name() == "Mine") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);

    // The mine should be registered in the TileRegistry.
    EXPECT_TRUE(state.registry().buildingAt(3, 3).has_value());
}

TEST(TurnResolverTest, ProductionIncludesBuildingYields) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    // Place a Mine on (2,2) which yields 2 production.
    state.addBuilding(makeMine(2, 2));

    // Enqueue a Farm (costs 20). Base(5) + mine(2) = 7 per turn.
    state.mutableCities()[0].buildQueue().enqueue(makeFarm, 3, 3);

    resolveTurn(state);

    // 7 production applied after 1 turn.
    EXPECT_EQ(state.mutableCities()[0].buildQueue().accumulatedProduction(), 7);
}

// ---------------------------------------------------------------------------
// Food and population growth
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, FoodAccumulatesAsSurplus) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    // Farm yields 2 food.
    state.addBuilding(makeFarm(2, 2));

    resolveTurn(state);

    EXPECT_EQ(state.cities()[0].foodSurplus(), 2);
}

TEST(TurnResolverTest, PopulationGrowsWhenThresholdMet) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    // Population starts at 1. Threshold = 10 + 5*1 = 15.
    EXPECT_EQ(state.cities()[0].population(), 1);

    state.addBuilding(makeFarm(2, 2));

    // Farm yields 2 food per turn. Need 8 turns to reach 16 >= 15.
    for (int i = 0; i < 7; ++i) {
        resolveTurn(state);
    }
    // After 7 turns: 14 food surplus, still pop 1.
    EXPECT_EQ(state.cities()[0].population(), 1);
    EXPECT_EQ(state.cities()[0].foodSurplus(), 14);

    resolveTurn(state); // 8th turn: 16 >= 15, pop grows to 2.
    EXPECT_EQ(state.cities()[0].population(), 2);
    // Surplus after growth: 16 - 15 = 1.
    EXPECT_EQ(state.cities()[0].foodSurplus(), 1);
}

TEST(TurnResolverTest, GrowthThresholdFormulaIsCorrect) {
    // threshold = 10 + 5 * population
    EXPECT_EQ(City::growthThreshold(1), 15);
    EXPECT_EQ(City::growthThreshold(2), 20);
    EXPECT_EQ(City::growthThreshold(5), 35);
    EXPECT_EQ(City::growthThreshold(10), 60);
}

// ---------------------------------------------------------------------------
// Buildings outside city tiles do not contribute
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, BuildingsOutsideCityDoNotContribute) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    // City only owns tiles (1,1).
    state.addCity(std::move(city));

    // Place a Market on (5,5) which is NOT in the city.
    state.addBuilding(makeMarket(5, 5));

    resolveTurn(state);

    EXPECT_EQ(state.factionResources().gold, 0);
}

// ---------------------------------------------------------------------------
// Full turn resolution integration
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, FullTurnResolutionWithBuildings) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    city.addTile(3, 3);
    state.addCity(std::move(city));

    // Place a Market at (2,2) and a Farm at (3,3).
    state.addBuilding(makeMarket(2, 2));
    state.addBuilding(makeFarm(3, 3));

    // Add a warrior and consume its movement.
    auto warrior = std::make_unique<Warrior>(0, 0);
    warrior->moveTo(0, 1);
    state.addUnit(std::move(warrior));

    EXPECT_EQ(state.getTurn(), 1);

    resolveTurn(state);

    // Turn advanced.
    EXPECT_EQ(state.getTurn(), 2);
    // Gold collected from Market (3 gold).
    EXPECT_EQ(state.factionResources().gold, 3);
    // Food collected from Farm (2 food).
    EXPECT_EQ(state.cities()[0].foodSurplus(), 2);
    // Movement reset.
    EXPECT_EQ(state.units()[0]->movementRemaining(), state.units()[0]->movement());
}

// ---------------------------------------------------------------------------
// No cities, no buildings — just advance turn and reset units
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, EmptyStateJustAdvancesTurn) {
    auto state = makeTestState();
    resolveTurn(state);
    EXPECT_EQ(state.getTurn(), 2);
}

// ---------------------------------------------------------------------------
// Multiple cities accumulate independently
// ---------------------------------------------------------------------------

TEST(TurnResolverTest, MultipleCitiesAccumulateIndependently) {
    auto state = makeTestState();

    City city1("City1", 1, 1);
    city1.addTile(2, 2);
    state.addCity(std::move(city1));

    City city2("City2", 5, 5);
    city2.addTile(6, 6);
    state.addCity(std::move(city2));

    // Market in city1, Farm in city2.
    state.addBuilding(makeMarket(2, 2));
    state.addBuilding(makeFarm(6, 6));

    resolveTurn(state);

    // City1 gets no food (market has no food), City2 gets 2 food.
    EXPECT_EQ(state.cities()[0].foodSurplus(), 0);
    EXPECT_EQ(state.cities()[1].foodSurplus(), 2);

    // Faction gold should have the Market's 3 gold.
    EXPECT_EQ(state.factionResources().gold, 3);
}
