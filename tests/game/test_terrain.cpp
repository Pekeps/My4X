#include "game/Map.h"
#include "game/TerrainType.h"
#include "game/Tile.h"

#include <gtest/gtest.h>

#include <set>

TEST(TerrainTypeTest, TerrainNameReturnsCorrectStrings) {
    EXPECT_EQ(game::terrainName(game::TerrainType::Plains), "Plains");
    EXPECT_EQ(game::terrainName(game::TerrainType::Hills), "Hills");
    EXPECT_EQ(game::terrainName(game::TerrainType::Forest), "Forest");
    EXPECT_EQ(game::terrainName(game::TerrainType::Water), "Water");
    EXPECT_EQ(game::terrainName(game::TerrainType::Mountain), "Mountain");
}

TEST(TileTest, DefaultTerrainIsPlains) {
    game::Tile tile(0, 0);
    EXPECT_EQ(tile.terrainType(), game::TerrainType::Plains);
}

TEST(TileTest, TerrainCanBeSetViaConstructor) {
    game::Tile tile(1, 2, game::TerrainType::Mountain);
    EXPECT_EQ(tile.terrainType(), game::TerrainType::Mountain);
    EXPECT_EQ(tile.row(), 1);
    EXPECT_EQ(tile.col(), 2);
}

TEST(TileTest, AllTerrainTypesCanBeAssigned) {
    game::Tile plains(0, 0, game::TerrainType::Plains);
    game::Tile hills(0, 1, game::TerrainType::Hills);
    game::Tile forest(0, 2, game::TerrainType::Forest);
    game::Tile water(0, 3, game::TerrainType::Water);
    game::Tile mountain(0, 4, game::TerrainType::Mountain);

    EXPECT_EQ(plains.terrainType(), game::TerrainType::Plains);
    EXPECT_EQ(hills.terrainType(), game::TerrainType::Hills);
    EXPECT_EQ(forest.terrainType(), game::TerrainType::Forest);
    EXPECT_EQ(water.terrainType(), game::TerrainType::Water);
    EXPECT_EQ(mountain.terrainType(), game::TerrainType::Mountain);
}

TEST(MapTest, SeededMapHasDeterministicTerrain) {
    game::Map map1(10, 10, 42);
    game::Map map2(10, 10, 42);

    for (int row = 0; row < 10; ++row) {
        for (int col = 0; col < 10; ++col) {
            EXPECT_EQ(map1.tile(row, col).terrainType(), map2.tile(row, col).terrainType());
        }
    }
}

TEST(MapTest, DifferentSeedsProduceDifferentMaps) {
    game::Map map1(10, 10, 1);
    game::Map map2(10, 10, 999);

    bool anyDifference = false;
    for (int row = 0; row < 10; ++row) {
        for (int col = 0; col < 10; ++col) {
            if (map1.tile(row, col).terrainType() != map2.tile(row, col).terrainType()) {
                anyDifference = true;
                break;
            }
        }
        if (anyDifference) {
            break;
        }
    }
    EXPECT_TRUE(anyDifference);
}

TEST(MapTest, SeededMapContainsMultipleTerrainTypes) {
    game::Map map(20, 20, 12345);

    std::set<game::TerrainType> found;
    for (int row = 0; row < 20; ++row) {
        for (int col = 0; col < 20; ++col) {
            found.insert(map.tile(row, col).terrainType());
        }
    }
    // With 400 tiles and 5 terrain types, we expect all types present
    EXPECT_EQ(found.size(), 5U);
}

TEST(MapTest, TilesHaveCorrectCoordinates) {
    game::Map map(5, 5, 42);
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            EXPECT_EQ(map.tile(row, col).row(), row);
            EXPECT_EQ(map.tile(row, col).col(), col);
        }
    }
}
