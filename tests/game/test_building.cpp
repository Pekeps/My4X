// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
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
    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Desert));
    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Swamp));
}

TEST(BuildingTest, TerrainRestrictionEnforced) {
    game::Building building("HillsOnly", {}, {}, {{0, 0}}, {game::TerrainType::Hills});

    EXPECT_TRUE(building.canBuildOn(game::TerrainType::Hills));
    EXPECT_FALSE(building.canBuildOn(game::TerrainType::Plains));
    EXPECT_FALSE(building.canBuildOn(game::TerrainType::Water));
}

// ── Model key ───────────────────────────────────────────────────────────

TEST(BuildingTest, ModelKeyDefaultsToEmpty) {
    game::Building building("NoModel", {}, {}, {{0, 0}});
    EXPECT_TRUE(building.modelKey().empty());
}

TEST(BuildingTest, ModelKeySetViaConstructor) {
    game::Building building("WithModel", {}, {}, {{0, 0}}, {}, "building_test");
    EXPECT_EQ(building.modelKey(), "building_test");
}

// ── Faction ownership ───────────────────────────────────────────────────

TEST(BuildingTest, FactionIdDefaultsToZero) {
    game::Building building("Unowned", {}, {}, {{0, 0}});
    EXPECT_EQ(building.factionId(), 0U);
}

TEST(BuildingTest, FactionIdSetViaConstructor) {
    game::Building building("Owned", {}, {}, {{0, 0}}, {}, "", 5);
    EXPECT_EQ(building.factionId(), 5U);
}

TEST(BuildingTest, FactionIdSetViaSetter) {
    game::Building building("Owned", {}, {}, {{0, 0}});
    building.setFactionId(3);
    EXPECT_EQ(building.factionId(), 3U);
}

// ── Under construction ──────────────────────────────────────────────────

TEST(BuildingTest, UnderConstructionDefaultsFalse) {
    game::Building building("Built", {}, {}, {{0, 0}});
    EXPECT_FALSE(building.underConstruction());
}

TEST(BuildingTest, UnderConstructionSetViaSetter) {
    game::Building building("WIP", {}, {}, {{0, 0}});
    building.setUnderConstruction(true);
    EXPECT_TRUE(building.underConstruction());
    building.setUnderConstruction(false);
    EXPECT_FALSE(building.underConstruction());
}

// ── Factory: City Center ────────────────────────────────────────────────

TEST(BuildingTest, CityCenterFactory) {
    auto cc = game::makeCityCenter(1, 1);

    EXPECT_EQ(cc.name(), "City Center");
    EXPECT_EQ(cc.yieldPerTurn().production, 1);
    EXPECT_EQ(cc.yieldPerTurn().food, 1);
    EXPECT_EQ(cc.occupiedTiles().size(), 1U);
    EXPECT_TRUE(cc.occupiedTiles().contains({1, 1}));
    EXPECT_TRUE(cc.allowedTerrains().empty());
    EXPECT_EQ(cc.modelKey(), "building_city_center");
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
    EXPECT_EQ(farm.modelKey(), "building_farm");
}

// ── Factory: Mine ────────────────────────────────────────────────────────

TEST(BuildingTest, MineFactory) {
    auto mine = game::makeMine(1, 2);

    EXPECT_EQ(mine.name(), "Mine");
    EXPECT_EQ(mine.yieldPerTurn(), (game::Resource{.gold = 0, .production = 2, .food = 0}));
    EXPECT_TRUE(mine.canBuildOn(game::TerrainType::Hills));
    EXPECT_FALSE(mine.canBuildOn(game::TerrainType::Plains));
    EXPECT_EQ(mine.modelKey(), "building_mine");
}

// ── Factory: Lumber Mill ────────────────────────────────────────────────

TEST(BuildingTest, LumberMillFactory) {
    auto lm = game::makeLumberMill(2, 3);

    EXPECT_EQ(lm.name(), "Lumber Mill");
    EXPECT_EQ(lm.yieldPerTurn(), (game::Resource{.gold = 0, .production = 2, .food = 0}));
    EXPECT_TRUE(lm.canBuildOn(game::TerrainType::Forest));
    EXPECT_FALSE(lm.canBuildOn(game::TerrainType::Plains));
    EXPECT_EQ(lm.modelKey(), "building_lumber_mill");
}

// ── Factory: Barracks ───────────────────────────────────────────────────

TEST(BuildingTest, BarracksFactory) {
    auto barracks = game::makeBarracks(4, 2);

    EXPECT_EQ(barracks.name(), "Barracks");
    EXPECT_EQ(barracks.yieldPerTurn(), (game::Resource{.gold = 0, .production = 0, .food = 0}));
    EXPECT_EQ(barracks.constructionCost().production, 25);
    EXPECT_TRUE(barracks.allowedTerrains().empty());
    EXPECT_EQ(barracks.modelKey(), "building_barracks");
}

// ── Factory: Market ──────────────────────────────────────────────────────

TEST(BuildingTest, MarketFactory) {
    auto market = game::makeMarket(4, 4);

    EXPECT_EQ(market.name(), "Market");
    EXPECT_EQ(market.yieldPerTurn(), (game::Resource{.gold = 3, .production = 0, .food = 0}));
    EXPECT_TRUE(market.allowedTerrains().empty());
    EXPECT_EQ(market.modelKey(), "building_market");
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
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
