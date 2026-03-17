#include "game/Building.h"

#include <gtest/gtest.h>

// ── Construction & getters ───────────────────────────────────────────────

TEST(BuildingTest, ConstructionAndGetters) {
    game::Building building("TestBuilding", {.gold = 1, .production = 2, .food = 3},
                            {.gold = 10, .production = 20, .food = 0}, {{2, 3}});

    EXPECT_EQ(building.name(), "TestBuilding");
    EXPECT_EQ(building.yieldPerTurn(), (game::Resource{.gold = 1, .production = 2, .food = 3}));
    EXPECT_EQ(building.constructionCost(), (game::Resource{.gold = 10, .production = 20, .food = 0}));
    EXPECT_EQ(building.occupiedTiles().size(), 1U);
    EXPECT_TRUE(building.occupiedTiles().contains({2, 3}));
}

TEST(BuildingTest, MultiTileBuilding) {
    std::set<game::TileCoord> tiles = {{0, 0}, {0, 1}, {1, 0}};
    game::Building building("Fortress", {.gold = 0, .production = 0, .food = 0},
                            {.gold = 50, .production = 50, .food = 0}, tiles);

    EXPECT_EQ(building.occupiedTiles().size(), 3U);
    EXPECT_TRUE(building.occupiedTiles().contains({0, 0}));
    EXPECT_TRUE(building.occupiedTiles().contains({0, 1}));
    EXPECT_TRUE(building.occupiedTiles().contains({1, 0}));
}

// ── Terrain restrictions ─────────────────────────────────────────────────

TEST(BuildingTest, NoTerrainRestrictionAllowsAll) {
    game::Building building("Open", {}, {}, {{0, 0}});

    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Plains));
    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Hills));
    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Forest));
    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Water));
    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Mountain));
}

TEST(BuildingTest, TerrainRestrictionEnforced) {
    game::Building building("HillsOnly", {}, {}, {{0, 0}}, {game::TerrainType::Hills});

    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Hills));
    EXPECT_FALSE(building.canBuildOn(game::TerrainType::Plains));
    EXPECT_FALSE(building.canBuildOn(game::TerrainType::Water));
}

// ── Factory: Farm ────────────────────────────────────────────────────────

TEST(BuildingTest, FarmFactory) {
    auto farm = game::makeFarm(3, 5);

    EXPECT_EQ(farm.name(), "Farm");
    EXPECT_EQ(farm.yieldPerTurn(), (game::Resource{.gold = 0, .production = 0, .food = 2}));
    EXPECT_EQ(farm.occupiedTiles().size(), 1U);
    EXPECT_TRUE(farm.occupiedTiles().contains({3, 5}));
    EXPECT_TRUE(farm.allowedTerrains().empty());
    EXPECT_TRUE(farm.canBuildOn(game::TerrainType::Plains));
}

// ── Factory: Mine ────────────────────────────────────────────────────────

TEST(BuildingTest, MineFactory) {
    auto mine = game::makeMine(1, 2);

    EXPECT_EQ(mine.name(), "Mine");
    EXPECT_EQ(mine.yieldPerTurn(), (game::Resource{.gold = 0, .production = 2, .food = 0}));
    EXPECT_TRUE(mine.canBuildOn(game::TerrainType::Hills));
    EXPECT_FALSE(mine.canBuildOn(game::TerrainType::Plains));
}

// ── Factory: Market ──────────────────────────────────────────────────────

TEST(BuildingTest, MarketFactory) {
    auto market = game::makeMarket(4, 4);

    EXPECT_EQ(market.name(), "Market");
    EXPECT_EQ(market.yieldPerTurn(), (game::Resource{.gold = 3, .production = 0, .food = 0}));
    EXPECT_TRUE(market.allowedTerrains().empty());
}

// ── TileCoord ordering ───────────────────────────────────────────────────

TEST(TileCoordTest, EqualityAndOrdering) {
    game::TileCoord a{1, 2};
    game::TileCoord b{1, 2};
    game::TileCoord c{2, 0};

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_LT(a, c);
}
