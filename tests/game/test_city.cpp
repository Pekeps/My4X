#include "game/City.h"

#include <gtest/gtest.h>

TEST(CityTest, ConstructionDefaults) {
    game::City city("TestCity", 5, 3);
    EXPECT_EQ(city.name(), "TestCity");
    EXPECT_EQ(city.centerRow(), 5);
    EXPECT_EQ(city.centerCol(), 3);
    EXPECT_EQ(city.factionId(), 0);
    EXPECT_EQ(city.population(), 1);
}

TEST(CityTest, ConstructionWithFaction) {
    game::City city("Capital", 0, 0, 2);
    EXPECT_EQ(city.factionId(), 2);
}

TEST(CityTest, CenterTileInFootprint) {
    game::City city("TestCity", 5, 3);
    EXPECT_TRUE(city.containsTile(5, 3));
    EXPECT_EQ(city.tiles().size(), 1);
}

TEST(CityTest, AddTileExpandsFootprint) {
    game::City city("TestCity", 5, 3);
    EXPECT_TRUE(city.addTile(5, 4));
    EXPECT_TRUE(city.containsTile(5, 4));
    EXPECT_EQ(city.tiles().size(), 2);
}

TEST(CityTest, AddDuplicateTileReturnsFalse) {
    game::City city("TestCity", 5, 3);
    EXPECT_TRUE(city.addTile(5, 4));
    EXPECT_FALSE(city.addTile(5, 4));
    EXPECT_EQ(city.tiles().size(), 2);
}

TEST(CityTest, AddCenterTileAgainReturnsFalse) {
    game::City city("TestCity", 5, 3);
    EXPECT_FALSE(city.addTile(5, 3));
    EXPECT_EQ(city.tiles().size(), 1);
}

TEST(CityTest, ContainsTileReturnsFalseForUnownedTile) {
    game::City city("TestCity", 5, 3);
    EXPECT_FALSE(city.containsTile(0, 0));
}

TEST(CityTest, GrowPopulation) {
    game::City city("TestCity", 5, 3);
    city.growPopulation(3);
    EXPECT_EQ(city.population(), 4);
    city.growPopulation(2);
    EXPECT_EQ(city.population(), 6);
}

TEST(CityTest, GrowPopulationByZero) {
    game::City city("TestCity", 5, 3);
    city.growPopulation(0);
    EXPECT_EQ(city.population(), 1);
}

TEST(CityTest, MultipleTilesExpansion) {
    game::City city("TestCity", 5, 3);
    city.addTile(5, 4);
    city.addTile(4, 3);
    city.addTile(6, 3);
    EXPECT_EQ(city.tiles().size(), 4);
    EXPECT_TRUE(city.containsTile(5, 3));
    EXPECT_TRUE(city.containsTile(5, 4));
    EXPECT_TRUE(city.containsTile(4, 3));
    EXPECT_TRUE(city.containsTile(6, 3));
}
