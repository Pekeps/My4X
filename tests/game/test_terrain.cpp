// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/HexDirection.h"
#include "game/Map.h"
#include "game/TerrainType.h"
#include "game/Tile.h"

#include <gtest/gtest.h>

#include <set>

// ── terrainName tests ────────────────────────────────────────────────────────

TEST(TerrainTypeTest, TerrainNameReturnsCorrectStrings) {
    EXPECT_EQ(game::terrainName(game::TerrainType::Plains), "Plains");
    EXPECT_EQ(game::terrainName(game::TerrainType::Hills), "Hills");
    EXPECT_EQ(game::terrainName(game::TerrainType::Forest), "Forest");
    EXPECT_EQ(game::terrainName(game::TerrainType::Water), "Water");
    EXPECT_EQ(game::terrainName(game::TerrainType::Mountain), "Mountain");
    EXPECT_EQ(game::terrainName(game::TerrainType::Desert), "Desert");
    EXPECT_EQ(game::terrainName(game::TerrainType::Swamp), "Swamp");
}

// ── Tile terrain storage ─────────────────────────────────────────────────────

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
    game::Tile desert(0, 5, game::TerrainType::Desert);
    game::Tile swamp(0, 6, game::TerrainType::Swamp);

    EXPECT_EQ(plains.terrainType(), game::TerrainType::Plains);
    EXPECT_EQ(hills.terrainType(), game::TerrainType::Hills);
    EXPECT_EQ(forest.terrainType(), game::TerrainType::Forest);
    EXPECT_EQ(water.terrainType(), game::TerrainType::Water);
    EXPECT_EQ(mountain.terrainType(), game::TerrainType::Mountain);
    EXPECT_EQ(desert.terrainType(), game::TerrainType::Desert);
    EXPECT_EQ(swamp.terrainType(), game::TerrainType::Swamp);
}

// ── TerrainProperties lookup tests ───────────────────────────────────────────

TEST(TerrainPropertiesTest, PlainsProperties) {
    const auto &props = game::getTerrainProperties(game::TerrainType::Plains);
    EXPECT_EQ(props.movementCost, 1);
    EXPECT_EQ(props.defenseModifier, 0);
    EXPECT_TRUE(props.passable);
    EXPECT_TRUE(props.buildable);
}

TEST(TerrainPropertiesTest, HillsProperties) {
    const auto &props = game::getTerrainProperties(game::TerrainType::Hills);
    EXPECT_EQ(props.movementCost, 2);
    EXPECT_EQ(props.defenseModifier, 3);
    EXPECT_TRUE(props.passable);
    EXPECT_TRUE(props.buildable);
}

TEST(TerrainPropertiesTest, ForestProperties) {
    const auto &props = game::getTerrainProperties(game::TerrainType::Forest);
    EXPECT_EQ(props.movementCost, 2);
    EXPECT_EQ(props.defenseModifier, 2);
    EXPECT_TRUE(props.passable);
    EXPECT_TRUE(props.buildable);
}

TEST(TerrainPropertiesTest, WaterProperties) {
    const auto &props = game::getTerrainProperties(game::TerrainType::Water);
    EXPECT_EQ(props.movementCost, 0);
    EXPECT_EQ(props.defenseModifier, 0);
    EXPECT_FALSE(props.passable);
    EXPECT_FALSE(props.buildable);
}

TEST(TerrainPropertiesTest, MountainProperties) {
    const auto &props = game::getTerrainProperties(game::TerrainType::Mountain);
    EXPECT_EQ(props.movementCost, 0);
    EXPECT_EQ(props.defenseModifier, 0);
    EXPECT_FALSE(props.passable);
    EXPECT_FALSE(props.buildable);
}

TEST(TerrainPropertiesTest, DesertProperties) {
    const auto &props = game::getTerrainProperties(game::TerrainType::Desert);
    EXPECT_EQ(props.movementCost, 1);
    EXPECT_EQ(props.defenseModifier, 0);
    EXPECT_TRUE(props.passable);
    EXPECT_TRUE(props.buildable);
}

TEST(TerrainPropertiesTest, SwampProperties) {
    const auto &props = game::getTerrainProperties(game::TerrainType::Swamp);
    EXPECT_EQ(props.movementCost, 3);
    EXPECT_EQ(props.defenseModifier, -1);
    EXPECT_TRUE(props.passable);
    EXPECT_TRUE(props.buildable);
}

TEST(TerrainPropertiesTest, AllTerrainTypesHaveProperties) {
    // Verify every terrain type has a valid entry in the properties table.
    EXPECT_NO_THROW((void)game::getTerrainProperties(game::TerrainType::Plains));
    EXPECT_NO_THROW((void)game::getTerrainProperties(game::TerrainType::Hills));
    EXPECT_NO_THROW((void)game::getTerrainProperties(game::TerrainType::Forest));
    EXPECT_NO_THROW((void)game::getTerrainProperties(game::TerrainType::Water));
    EXPECT_NO_THROW((void)game::getTerrainProperties(game::TerrainType::Mountain));
    EXPECT_NO_THROW((void)game::getTerrainProperties(game::TerrainType::Desert));
    EXPECT_NO_THROW((void)game::getTerrainProperties(game::TerrainType::Swamp));
}

TEST(TerrainPropertiesTest, ImpassableTerrainHasZeroMovementCost) {
    // Impassable terrains should have movementCost == 0 as a sentinel.
    const auto &water = game::getTerrainProperties(game::TerrainType::Water);
    const auto &mountain = game::getTerrainProperties(game::TerrainType::Mountain);
    EXPECT_EQ(water.movementCost, 0);
    EXPECT_EQ(mountain.movementCost, 0);
    EXPECT_FALSE(water.passable);
    EXPECT_FALSE(mountain.passable);
}

TEST(TerrainPropertiesTest, PassableTerrainHasPositiveMovementCost) {
    // All passable terrains should have a positive movement cost.
    const auto &plains = game::getTerrainProperties(game::TerrainType::Plains);
    const auto &hills = game::getTerrainProperties(game::TerrainType::Hills);
    const auto &forest = game::getTerrainProperties(game::TerrainType::Forest);
    const auto &desert = game::getTerrainProperties(game::TerrainType::Desert);
    const auto &swamp = game::getTerrainProperties(game::TerrainType::Swamp);
    EXPECT_GT(plains.movementCost, 0);
    EXPECT_GT(hills.movementCost, 0);
    EXPECT_GT(forest.movementCost, 0);
    EXPECT_GT(desert.movementCost, 0);
    EXPECT_GT(swamp.movementCost, 0);
}

TEST(TerrainPropertiesTest, TerrainTypeCountMatchesEnum) {
    // Sanity check that TERRAIN_TYPE_COUNT matches the properties table size.
    EXPECT_EQ(game::TERRAIN_TYPE_COUNT, game::TERRAIN_PROPERTIES.size());
}

// ── Map generation tests ─────────────────────────────────────────────────────

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
    // With 400 tiles and 7 terrain types, we expect all types present
    EXPECT_EQ(found.size(), 7U);
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

// ── Tile elevation tests ──────────────────────────────────────────────────────

TEST(TileTest, DefaultElevation) {
    game::Tile plains(0, 0, game::TerrainType::Plains);
    EXPECT_EQ(plains.elevation(), 0);
}

TEST(TileTest, SetAndGetElevation) {
    game::Tile tile(0, 0, game::TerrainType::Plains);
    tile.setElevation(3);
    EXPECT_EQ(tile.elevation(), 3);
}

TEST(TileTest, MapGenerationSetsElevationFromTerrain) {
    game::Map map(5, 5, 42);
    for (int r = 0; r < map.height(); ++r) {
        for (int c = 0; c < map.width(); ++c) {
            const auto &t = map.tile(r, c);
            if (t.terrainType() == game::TerrainType::Water) {
                EXPECT_EQ(t.elevation(), 0);
            } else if (t.terrainType() == game::TerrainType::Mountain) {
                EXPECT_EQ(t.elevation(), 3);
            }
            EXPECT_GE(t.elevation(), 0);
        }
    }
}
// ── River tests ───────────────────────────────────────────────────────────────

TEST(TileTest, NoRiverByDefault) {
    game::Tile t(0, 0);
    EXPECT_FALSE(t.hasRiver());
    EXPECT_FALSE(t.hasIncomingRiver());
    EXPECT_FALSE(t.hasOutgoingRiver());
    EXPECT_FALSE(t.hasRiverBeginOrEnd());
}

TEST(TileTest, SetOutgoingRiver) {
    game::Tile t(0, 0);
    t.setOutgoingRiver(game::HexDirection::E);
    EXPECT_TRUE(t.hasOutgoingRiver());
    EXPECT_TRUE(t.hasRiver());
    EXPECT_TRUE(t.hasRiverBeginOrEnd());
    EXPECT_EQ(t.outgoingRiverDirection(), game::HexDirection::E);
    EXPECT_TRUE(t.hasRiverThroughEdge(game::HexDirection::E));
    EXPECT_FALSE(t.hasRiverThroughEdge(game::HexDirection::NE));
}

TEST(TileTest, SetIncomingRiver) {
    game::Tile t(0, 0);
    t.setIncomingRiver(game::HexDirection::SW);
    EXPECT_TRUE(t.hasIncomingRiver());
    EXPECT_TRUE(t.hasRiver());
    EXPECT_TRUE(t.hasRiverBeginOrEnd());
}

TEST(TileTest, BothRivers) {
    game::Tile t(0, 0);
    t.setIncomingRiver(game::HexDirection::NE);
    t.setOutgoingRiver(game::HexDirection::SW);
    EXPECT_TRUE(t.hasRiver());
    EXPECT_FALSE(t.hasRiverBeginOrEnd());
}

TEST(TileTest, RemoveRiver) {
    game::Tile t(0, 0);
    t.setOutgoingRiver(game::HexDirection::E);
    t.removeOutgoingRiver();
    EXPECT_FALSE(t.hasOutgoingRiver());
    EXPECT_FALSE(t.hasRiver());
}

TEST(TileTest, WaterLevel) {
    game::Tile t(0, 0, game::TerrainType::Water);
    t.setElevation(0);
    t.setWaterLevel(1);
    EXPECT_TRUE(t.isUnderwater());
    EXPECT_EQ(t.waterLevel(), 1);
}

TEST(TileTest, NotUnderwater) {
    game::Tile t(0, 0, game::TerrainType::Plains);
    t.setElevation(2);
    t.setWaterLevel(1);
    EXPECT_FALSE(t.isUnderwater());
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
