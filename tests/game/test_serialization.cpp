#include "game/Building.h"
#include "game/City.h"
#include "game/GameState.h"
#include "game/Resource.h"
#include "game/Serialization.h"

#include <gtest/gtest.h>

TEST(SerializationTest, EmptyGameStateRoundTrip) {
    game::GameState state(5, 5);
    EXPECT_EQ(state.getTurn(), 1);

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    EXPECT_EQ(restored.getTurn(), 1);
    EXPECT_EQ(restored.cities().size(), 0U);
    EXPECT_EQ(restored.buildings().size(), 0U);
    EXPECT_EQ(restored.units().size(), 0U);
}

TEST(SerializationTest, GameStateWithCityBuildingResourcesRoundTrip) {
    game::GameState state(10, 10);

    // Add a city
    game::City city("Rome", 3, 4, 1);
    city.growPopulation(4); // population becomes 5
    city.addTile(3, 4);
    city.addTile(3, 5);
    state.addCity(std::move(city));

    // Add a building
    game::Building farm = game::makeFarm(2, 2);
    state.addBuilding(std::move(farm));

    // Set faction resources
    state.factionResources().gold = 42;
    state.factionResources().production = 10;
    state.factionResources().food = 7;

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    ASSERT_EQ(restored.cities().size(), 1U);
    const game::City &restoredCity = restored.cities()[0];
    EXPECT_EQ(restoredCity.name(), "Rome");
    EXPECT_EQ(restoredCity.population(), 5);
    EXPECT_EQ(restoredCity.centerRow(), 3);
    EXPECT_EQ(restoredCity.centerCol(), 4);
    EXPECT_EQ(restoredCity.factionId(), 1);

    ASSERT_EQ(restored.buildings().size(), 1U);
    const game::Building &restoredBuilding = restored.buildings()[0];
    EXPECT_EQ(restoredBuilding.name(), "Farm");

    EXPECT_EQ(restored.factionResources().gold, 42);
    EXPECT_EQ(restored.factionResources().production, 10);
    EXPECT_EQ(restored.factionResources().food, 7);
}
