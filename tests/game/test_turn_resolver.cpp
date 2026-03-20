// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "game/TurnResolver.h"

#include "game/Building.h"
#include "game/City.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>
#include <memory>

using namespace game;

static GameState makeTestState() { return GameState(10, 10); }

static UnitTypeRegistry makeTestRegistry() {
    UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

TEST(TurnResolverTest, AdvancesTurnCounter) {
    auto state = makeTestState();
    EXPECT_EQ(state.getTurn(), 1);
    resolveTurn(state);
    EXPECT_EQ(state.getTurn(), 2);
    resolveTurn(state);
    EXPECT_EQ(state.getTurn(), 3);
}

TEST(TurnResolverTest, ResetsUnitMovement) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();
    auto warrior = std::make_unique<Warrior>(0, 0, reg, 1);
    warrior->moveTo(0, 1, 1);
    EXPECT_LT(warrior->movementRemaining(), warrior->movement());
    state.addUnit(std::move(warrior));

    resolveTurn(state);

    EXPECT_EQ(state.units()[0]->movementRemaining(), state.units()[0]->movement());
}

TEST(TurnResolverTest, CollectsGoldFromBuildingsToFaction) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    state.addBuilding(makeMarket(2, 2));

    EXPECT_EQ(state.factionResources().gold, 0);

    resolveTurn(state);

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

TEST(TurnResolverTest, AppliesProductionToBuildQueue) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    state.mutableCities()[0].buildQueue().enqueue(makeMine, 3, 3);

    resolveTurn(state);

    EXPECT_EQ(state.mutableCities()[0].buildQueue().accumulatedProduction(), 5);
    EXPECT_FALSE(state.mutableCities()[0].buildQueue().isEmpty());
}

TEST(TurnResolverTest, BuildingCompletesAndGetsPlaced) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    state.mutableCities()[0].buildQueue().enqueue(makeMine, 3, 3);

    resolveTurn(state);
    EXPECT_FALSE(state.mutableCities()[0].buildQueue().isEmpty());

    resolveTurn(state);
    EXPECT_TRUE(state.mutableCities()[0].buildQueue().isEmpty());

    bool found = false;
    for (const Building &b : state.buildings()) {
        if (b.name() == "Mine") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);

    EXPECT_TRUE(state.registry().buildingAt(3, 3).has_value());
}

TEST(TurnResolverTest, ProductionIncludesBuildingYields) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    state.addBuilding(makeMine(2, 2));

    state.mutableCities()[0].buildQueue().enqueue(makeFarm, 3, 3);

    resolveTurn(state);

    EXPECT_EQ(state.mutableCities()[0].buildQueue().accumulatedProduction(), 7);
}

TEST(TurnResolverTest, FoodAccumulatesAsSurplus) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    state.addBuilding(makeFarm(2, 2));

    resolveTurn(state);

    EXPECT_EQ(state.cities()[0].foodSurplus(), 2);
}

TEST(TurnResolverTest, PopulationGrowsWhenThresholdMet) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    EXPECT_EQ(state.cities()[0].population(), 1);

    state.addBuilding(makeFarm(2, 2));

    for (int i = 0; i < 7; ++i) {
        resolveTurn(state);
    }
    EXPECT_EQ(state.cities()[0].population(), 1);
    EXPECT_EQ(state.cities()[0].foodSurplus(), 14);

    resolveTurn(state);
    EXPECT_EQ(state.cities()[0].population(), 2);
    EXPECT_EQ(state.cities()[0].foodSurplus(), 1);
}

TEST(TurnResolverTest, GrowthThresholdFormulaIsCorrect) {
    EXPECT_EQ(City::growthThreshold(1), 15);
    EXPECT_EQ(City::growthThreshold(2), 20);
    EXPECT_EQ(City::growthThreshold(5), 35);
    EXPECT_EQ(City::growthThreshold(10), 60);
}

TEST(TurnResolverTest, BuildingsOutsideCityDoNotContribute) {
    auto state = makeTestState();

    City city("TestCity", 1, 1);
    state.addCity(std::move(city));

    state.addBuilding(makeMarket(5, 5));

    resolveTurn(state);

    EXPECT_EQ(state.factionResources().gold, 0);
}

TEST(TurnResolverTest, FullTurnResolutionWithBuildings) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    city.addTile(3, 3);
    state.addCity(std::move(city));

    state.addBuilding(makeMarket(2, 2));
    state.addBuilding(makeFarm(3, 3));

    auto warrior = std::make_unique<Warrior>(0, 0, reg, 1);
    warrior->moveTo(0, 1, 1);
    state.addUnit(std::move(warrior));

    EXPECT_EQ(state.getTurn(), 1);

    resolveTurn(state);

    EXPECT_EQ(state.getTurn(), 2);
    EXPECT_EQ(state.factionResources().gold, 3);
    EXPECT_EQ(state.cities()[0].foodSurplus(), 2);
    EXPECT_EQ(state.units()[0]->movementRemaining(), state.units()[0]->movement());
}

TEST(TurnResolverTest, EmptyStateJustAdvancesTurn) {
    auto state = makeTestState();
    resolveTurn(state);
    EXPECT_EQ(state.getTurn(), 2);
}

TEST(TurnResolverTest, MultipleCitiesAccumulateIndependently) {
    auto state = makeTestState();

    City city1("City1", 1, 1);
    city1.addTile(2, 2);
    state.addCity(std::move(city1));

    City city2("City2", 5, 5);
    city2.addTile(6, 6);
    state.addCity(std::move(city2));

    state.addBuilding(makeMarket(2, 2));
    state.addBuilding(makeFarm(6, 6));

    resolveTurn(state);

    EXPECT_EQ(state.cities()[0].foodSurplus(), 0);
    EXPECT_EQ(state.cities()[1].foodSurplus(), 2);

    EXPECT_EQ(state.factionResources().gold, 3);
}

// -- Faction-aware resource routing tests ------------------------------

TEST(TurnResolverTest, GoldRoutesToRegisteredFaction) {
    auto state = makeTestState();

    // Register a faction
    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);

    // Create a city owned by this faction
    City city("Rome", 1, 1, static_cast<int>(fid));
    city.addTile(2, 2);
    state.addCity(std::move(city));

    state.addBuilding(makeMarket(2, 2));

    EXPECT_EQ(state.factionRegistry().getFaction(fid).stockpile().gold, 0);

    resolveTurn(state);

    // Gold should go to the faction's stockpile, not the legacy one
    EXPECT_EQ(state.factionRegistry().getFaction(fid).stockpile().gold, 3);
    EXPECT_EQ(state.factionResources().gold, 0);
}

TEST(TurnResolverTest, GoldRoutesToCorrectFactionForMultipleFactions) {
    auto state = makeTestState();

    FactionId fid1 = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    FactionId fid2 = state.mutableFactionRegistry().addFaction("Greece", FactionType::Player, 1);

    // City 1 owned by faction 1 with a market
    City city1("Rome", 1, 1, static_cast<int>(fid1));
    city1.addTile(2, 2);
    state.addCity(std::move(city1));
    state.addBuilding(makeMarket(2, 2));

    // City 2 owned by faction 2 with a market
    City city2("Athens", 5, 5, static_cast<int>(fid2));
    city2.addTile(6, 6);
    state.addCity(std::move(city2));
    state.addBuilding(makeMarket(6, 6));

    resolveTurn(state);

    EXPECT_EQ(state.factionRegistry().getFaction(fid1).stockpile().gold, 3);
    EXPECT_EQ(state.factionRegistry().getFaction(fid2).stockpile().gold, 3);
    EXPECT_EQ(state.factionResources().gold, 0);
}

TEST(TurnResolverTest, GoldAccumulatesAcrossTurnsForFaction) {
    auto state = makeTestState();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);

    City city("Rome", 1, 1, static_cast<int>(fid));
    city.addTile(2, 2);
    state.addCity(std::move(city));
    state.addBuilding(makeMarket(2, 2));

    resolveTurn(state);
    resolveTurn(state);

    EXPECT_EQ(state.factionRegistry().getFaction(fid).stockpile().gold, 6);
}

TEST(TurnResolverTest, CapturedCityRoutesToNewFaction) {
    auto state = makeTestState();

    FactionId fid1 = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    FactionId fid2 = state.mutableFactionRegistry().addFaction("Greece", FactionType::Player, 1);

    City city("Disputed", 1, 1, static_cast<int>(fid1));
    city.addTile(2, 2);
    state.addCity(std::move(city));
    state.addBuilding(makeMarket(2, 2));

    // First turn: gold goes to faction 1
    resolveTurn(state);
    EXPECT_EQ(state.factionRegistry().getFaction(fid1).stockpile().gold, 3);
    EXPECT_EQ(state.factionRegistry().getFaction(fid2).stockpile().gold, 0);

    // Capture: city changes ownership to faction 2
    state.mutableCities()[0].setFactionId(static_cast<int>(fid2));

    // Second turn: gold goes to faction 2
    resolveTurn(state);
    EXPECT_EQ(state.factionRegistry().getFaction(fid1).stockpile().gold, 3);
    EXPECT_EQ(state.factionRegistry().getFaction(fid2).stockpile().gold, 3);
}

TEST(TurnResolverTest, FactionTotalIncomeIsSumOfCityOutputs) {
    auto state = makeTestState();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);

    // Two cities owned by the same faction, each with a market
    City city1("Rome", 1, 1, static_cast<int>(fid));
    city1.addTile(2, 2);
    state.addCity(std::move(city1));
    state.addBuilding(makeMarket(2, 2));

    City city2("Pompeii", 5, 5, static_cast<int>(fid));
    city2.addTile(6, 6);
    state.addCity(std::move(city2));
    state.addBuilding(makeMarket(6, 6));

    resolveTurn(state);

    // Total = 3 + 3 = 6
    EXPECT_EQ(state.factionRegistry().getFaction(fid).stockpile().gold, 6);
}

TEST(TurnResolverTest, UnregisteredFactionFallsBackToLegacy) {
    auto state = makeTestState();

    // City with factionId=0 (no registered faction) — legacy behavior
    City city("Unowned", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));
    state.addBuilding(makeMarket(2, 2));

    resolveTurn(state);

    EXPECT_EQ(state.factionResources().gold, 3);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
