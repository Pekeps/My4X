#include "game/GameState.h"
#include "game/Building.h"
#include "game/City.h"
#include "game/Resource.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

// -- Turn tests --------------------------------------------------------

TEST(GameStateTest, InitialTurnIsOne) {
    game::GameState state(4, 4);
    EXPECT_EQ(state.getTurn(), 1);
}

TEST(GameStateTest, NextTurnIncrements) {
    game::GameState state(4, 4);
    state.nextTurn();
    EXPECT_EQ(state.getTurn(), 2);
}

// -- Map tests ---------------------------------------------------------

TEST(GameStateTest, MapDimensions) {
    game::GameState state(10, 8);
    EXPECT_EQ(state.map().height(), 10);
    EXPECT_EQ(state.map().width(), 8);
}

// -- City tests --------------------------------------------------------

TEST(GameStateTest, AddCityRegistersInTileRegistry) {
    game::GameState state(4, 4);
    game::City city("TestCity", 1, 2);
    game::CityId id = state.addCity(city);

    EXPECT_EQ(state.cities().size(), 1);
    EXPECT_EQ(state.cities().front().name(), "TestCity");
    EXPECT_TRUE(state.registry().cityAt(1, 2).has_value());
    EXPECT_EQ(state.registry().cityAt(1, 2).value(), id);
}

TEST(GameStateTest, RemoveCityUnregistersFromTileRegistry) {
    game::GameState state(4, 4);
    game::City city("TestCity", 1, 2);
    game::CityId id = state.addCity(city);

    state.removeCity(id);
    EXPECT_TRUE(state.cities().empty());
    EXPECT_FALSE(state.registry().cityAt(1, 2).has_value());
}

TEST(GameStateTest, RemoveNonExistentCityThrows) {
    game::GameState state(4, 4);
    EXPECT_THROW(state.removeCity(999), std::out_of_range);
}

// -- City-by-faction queries -------------------------------------------

TEST(GameStateTest, CitiesForFactionReturnsMatchingCities) {
    game::GameState state(4, 4);
    state.addCity(game::City("Rome", 0, 0, 1));
    state.addCity(game::City("Athens", 1, 1, 2));
    state.addCity(game::City("Sparta", 2, 2, 1));

    auto faction1Cities = state.citiesForFaction(1);
    EXPECT_EQ(faction1Cities.size(), 2);
    EXPECT_EQ(faction1Cities[0]->name(), "Rome");
    EXPECT_EQ(faction1Cities[1]->name(), "Sparta");

    auto faction2Cities = state.citiesForFaction(2);
    EXPECT_EQ(faction2Cities.size(), 1);
    EXPECT_EQ(faction2Cities[0]->name(), "Athens");
}

TEST(GameStateTest, CitiesForFactionReturnsEmptyForUnknownFaction) {
    game::GameState state(4, 4);
    state.addCity(game::City("Rome", 0, 0, 1));
    auto result = state.citiesForFaction(99);
    EXPECT_TRUE(result.empty());
}

TEST(GameStateTest, MutableCitiesForFaction) {
    game::GameState state(4, 4);
    state.addCity(game::City("Rome", 0, 0, 1));
    state.addCity(game::City("Athens", 1, 1, 2));

    auto faction1Cities = state.mutableCitiesForFaction(1);
    ASSERT_EQ(faction1Cities.size(), 1);
    faction1Cities[0]->setFactionId(2);

    // After capture, faction 1 has no cities, faction 2 has both
    EXPECT_TRUE(state.citiesForFaction(1).empty());
    EXPECT_EQ(state.citiesForFaction(2).size(), 2);
}

// -- Building tests ----------------------------------------------------

TEST(GameStateTest, AddBuildingRegistersInTileRegistry) {
    game::GameState state(4, 4);
    auto farm = game::makeFarm(2, 3);
    game::BuildingId id = state.addBuilding(farm);

    EXPECT_EQ(state.buildings().size(), 1);
    EXPECT_EQ(state.buildings().front().name(), "Farm");
    EXPECT_TRUE(state.registry().buildingAt(2, 3).has_value());
    EXPECT_EQ(state.registry().buildingAt(2, 3).value(), id);
}

TEST(GameStateTest, RemoveBuildingUnregistersFromTileRegistry) {
    game::GameState state(4, 4);
    auto farm = game::makeFarm(2, 3);
    game::BuildingId id = state.addBuilding(farm);

    state.removeBuilding(id);
    EXPECT_TRUE(state.buildings().empty());
    EXPECT_FALSE(state.registry().buildingAt(2, 3).has_value());
}

TEST(GameStateTest, RemoveNonExistentBuildingThrows) {
    game::GameState state(4, 4);
    EXPECT_THROW(state.removeBuilding(999), std::out_of_range);
}

// -- Unit tests --------------------------------------------------------

TEST(GameStateTest, AddUnitRegistersInTileRegistry) {
    game::GameState state(4, 4);
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    auto warrior = std::make_unique<game::Warrior>(1, 1, reg);
    game::Unit *raw = warrior.get();

    std::size_t idx = state.addUnit(std::move(warrior));
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(state.units().size(), 1);

    auto unitsOnTile = state.registry().unitsAt(1, 1);
    EXPECT_EQ(unitsOnTile.size(), 1);
    EXPECT_EQ(unitsOnTile.front(), raw);
}

TEST(GameStateTest, RemoveUnitUnregistersFromTileRegistry) {
    game::GameState state(4, 4);
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    auto warrior = std::make_unique<game::Warrior>(1, 1, reg);
    state.addUnit(std::move(warrior));

    state.removeUnit(0);
    EXPECT_TRUE(state.units().empty());
    EXPECT_TRUE(state.registry().unitsAt(1, 1).empty());
}

TEST(GameStateTest, RemoveUnitOutOfRangeThrows) {
    game::GameState state(4, 4);
    EXPECT_THROW(state.removeUnit(0), std::out_of_range);
}

// -- Faction resource tests -------------------------------------------

TEST(GameStateTest, FactionResourcesDefaultToZero) {
    game::GameState state(4, 4);
    EXPECT_EQ(state.factionResources(), (game::Resource{.gold = 0, .production = 0, .food = 0}));
}

TEST(GameStateTest, FactionResourcesMutable) {
    game::GameState state(4, 4);
    state.factionResources() += game::Resource{.gold = 100, .production = 50, .food = 25};
    EXPECT_EQ(state.factionResources().gold, 100);
    EXPECT_EQ(state.factionResources().production, 50);
    EXPECT_EQ(state.factionResources().food, 25);
}
