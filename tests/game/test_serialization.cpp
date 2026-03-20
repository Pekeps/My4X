#include "game/Building.h"
#include "game/BuildQueue.h"
#include "game/City.h"
#include "game/GameState.h"
#include "game/Resource.h"
#include "game/Serialization.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

namespace {

game::UnitTypeRegistry makeTestRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

constexpr game::FactionId TEST_FACTION = 1;

} // namespace

TEST(SerializationTest, EmptyGameStateRoundTrip) {
    game::GameState state(5, 5, 42);
    EXPECT_EQ(state.getTurn(), 1);

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    EXPECT_EQ(restored.getTurn(), 1);
    EXPECT_EQ(restored.map().height(), 5);
    EXPECT_EQ(restored.map().width(), 5);
    EXPECT_EQ(restored.cities().size(), 0U);
    EXPECT_EQ(restored.buildings().size(), 0U);
    EXPECT_EQ(restored.units().size(), 0U);
}

TEST(SerializationTest, MapTerrainRoundTrip) {
    game::GameState state(8, 8, 12345);

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    EXPECT_EQ(restored.map().height(), 8);
    EXPECT_EQ(restored.map().width(), 8);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            EXPECT_EQ(restored.map().tile(row, col).terrainType(), state.map().tile(row, col).terrainType())
                << "Terrain mismatch at (" << row << ", " << col << ")";
        }
    }
}

TEST(SerializationTest, AllTerrainTypesRoundTrip) {
    game::GameState state(20, 20, 99);

    bool hasPlains = false, hasHills = false, hasForest = false, hasWater = false, hasMountain = false;
    bool hasDesert = false, hasSwamp = false;
    for (int r = 0; r < 20; ++r) {
        for (int c = 0; c < 20; ++c) {
            switch (state.map().tile(r, c).terrainType()) {
            case game::TerrainType::Plains:
                hasPlains = true;
                break;
            case game::TerrainType::Hills:
                hasHills = true;
                break;
            case game::TerrainType::Forest:
                hasForest = true;
                break;
            case game::TerrainType::Water:
                hasWater = true;
                break;
            case game::TerrainType::Mountain:
                hasMountain = true;
                break;
            case game::TerrainType::Desert:
                hasDesert = true;
                break;
            case game::TerrainType::Swamp:
                hasSwamp = true;
                break;
            }
        }
    }
    ASSERT_TRUE(hasPlains) << "Test setup: seed 99 should produce Plains tiles";
    ASSERT_TRUE(hasHills) << "Test setup: seed 99 should produce Hills tiles";
    ASSERT_TRUE(hasForest) << "Test setup: seed 99 should produce Forest tiles";
    ASSERT_TRUE(hasWater) << "Test setup: seed 99 should produce Water tiles";
    ASSERT_TRUE(hasMountain) << "Test setup: seed 99 should produce Mountain tiles";
    ASSERT_TRUE(hasDesert) << "Test setup: seed 99 should produce Desert tiles";
    ASSERT_TRUE(hasSwamp) << "Test setup: seed 99 should produce Swamp tiles";

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    for (int r = 0; r < 20; ++r) {
        for (int c = 0; c < 20; ++c) {
            EXPECT_EQ(restored.map().tile(r, c).terrainType(), state.map().tile(r, c).terrainType())
                << "Terrain mismatch at (" << r << ", " << c << ")";
        }
    }
}

TEST(SerializationTest, GameStateWithCityBuildingResourcesRoundTrip) {
    game::GameState state(10, 10, 42);

    game::City city("Rome", 3, 4, 1);
    city.growPopulation(4);
    city.addTile(3, 5);
    city.addTile(4, 4);
    state.addCity(std::move(city));

    game::Building mine = game::makeMine(2, 2);
    state.addBuilding(std::move(mine));

    game::Building farm = game::makeFarm(5, 5);
    state.addBuilding(std::move(farm));

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
    EXPECT_TRUE(restoredCity.containsTile(3, 4));
    EXPECT_TRUE(restoredCity.containsTile(3, 5));
    EXPECT_TRUE(restoredCity.containsTile(4, 4));
    EXPECT_EQ(restoredCity.tiles().size(), 3U);

    ASSERT_EQ(restored.buildings().size(), 2U);
    const game::Building &restoredMine = restored.buildings()[0];
    EXPECT_EQ(restoredMine.name(), "Mine");
    EXPECT_TRUE(restoredMine.canBuildOn(game::TerrainType::Hills));
    EXPECT_FALSE(restoredMine.canBuildOn(game::TerrainType::Plains));
    EXPECT_EQ(restoredMine.allowedTerrains().size(), 1U);

    const game::Building &restoredFarm = restored.buildings()[1];
    EXPECT_EQ(restoredFarm.name(), "Farm");
    EXPECT_TRUE(restoredFarm.canBuildOn(game::TerrainType::Plains));
    EXPECT_TRUE(restoredFarm.canBuildOn(game::TerrainType::Hills));
    EXPECT_EQ(restoredFarm.allowedTerrains().size(), 0U);

    EXPECT_EQ(restored.factionResources().gold, 42);
    EXPECT_EQ(restored.factionResources().production, 10);
    EXPECT_EQ(restored.factionResources().food, 7);
}

TEST(SerializationTest, UnitRoundTrip) {
    game::GameState state(10, 10, 42);
    auto reg = makeTestRegistry();

    auto warrior = std::make_unique<game::Warrior>(3, 4, reg, TEST_FACTION);
    warrior->takeDamage(30);
    warrior->moveTo(4, 4);
    state.addUnit(std::move(warrior));

    auto fullHealthWarrior = std::make_unique<game::Warrior>(7, 7, reg, TEST_FACTION);
    state.addUnit(std::move(fullHealthWarrior));

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    ASSERT_EQ(restored.units().size(), 2U);

    const auto &u0 = restored.units()[0];
    EXPECT_EQ(u0->name(), "Warrior");
    EXPECT_EQ(u0->row(), 4);
    EXPECT_EQ(u0->col(), 4);
    EXPECT_EQ(u0->health(), 70);
    EXPECT_EQ(u0->movementRemaining(), 1);

    const auto &u1 = restored.units()[1];
    EXPECT_EQ(u1->row(), 7);
    EXPECT_EQ(u1->col(), 7);
    EXPECT_EQ(u1->health(), 100);
    EXPECT_EQ(u1->movementRemaining(), 2);
}

TEST(SerializationTest, TurnCounterRoundTrip) {
    game::GameState state(5, 5, 42);
    for (int i = 0; i < 9; ++i) {
        state.nextTurn();
    }
    EXPECT_EQ(state.getTurn(), 10);

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    EXPECT_EQ(restored.getTurn(), 10);
}

TEST(SerializationTest, EntityIdPreservation) {
    game::GameState state(10, 10, 42);

    game::City city1("Rome", 1, 1, 0);
    game::City city2("Athens", 5, 5, 0);
    state.addCity(std::move(city1));
    state.addCity(std::move(city2));
    state.removeCity(1);

    game::Building b1 = game::makeFarm(2, 2);
    game::Building b2 = game::makeMarket(3, 3);
    state.addBuilding(std::move(b1));
    state.addBuilding(std::move(b2));
    state.removeBuilding(1);

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    ASSERT_EQ(restored.cities().size(), 1U);
    EXPECT_EQ(restored.cities()[0].id(), 2U);
    EXPECT_EQ(restored.cities()[0].name(), "Athens");

    ASSERT_EQ(restored.buildings().size(), 1U);
    EXPECT_EQ(restored.buildings()[0].id(), 2U);
    EXPECT_EQ(restored.buildings()[0].name(), "Market");

    game::City city3("Sparta", 8, 8, 0);
    game::CityId newId = restored.addCity(std::move(city3));
    EXPECT_GT(newId, 2U);
}

TEST(SerializationTest, BuildQueueRoundTrip) {
    game::GameState state(10, 10, 42);

    game::City city("Rome", 3, 4, 0);
    city.buildQueue().enqueue(game::makeFarm, 3, 5);
    city.buildQueue().enqueue(game::makeMine, 4, 4);
    city.buildQueue().applyProduction(8);
    state.addCity(std::move(city));

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    ASSERT_EQ(restored.cities().size(), 1U);
    const game::BuildQueue &bq = restored.cities()[0].buildQueue();
    EXPECT_FALSE(bq.isEmpty());
    EXPECT_EQ(bq.accumulatedProduction(), 8);

    auto current = bq.currentItem();
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->name, "Farm");
    EXPECT_EQ(current->targetRow, 3);
    EXPECT_EQ(current->targetCol, 5);
    EXPECT_EQ(current->productionCost, 20);
}

TEST(SerializationTest, TileRegistryRebuiltCorrectly) {
    game::GameState state(10, 10, 42);
    auto reg = makeTestRegistry();

    game::City city("Rome", 3, 4, 0);
    city.addTile(3, 5);
    city.addTile(4, 4);
    state.addCity(std::move(city));

    state.addBuilding(game::makeFarm(6, 6));
    state.addUnit(std::make_unique<game::Warrior>(7, 7, reg, TEST_FACTION));

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    EXPECT_TRUE(restored.registry().cityAt(3, 4).has_value());
    EXPECT_TRUE(restored.registry().cityAt(3, 5).has_value());
    EXPECT_TRUE(restored.registry().cityAt(4, 4).has_value());

    EXPECT_TRUE(restored.registry().buildingAt(6, 6).has_value());

    EXPECT_FALSE(restored.registry().unitsAt(7, 7).empty());
}

TEST(SerializationTest, CorruptInputThrows) {
    EXPECT_THROW((void)game::deserializeGameState(""), std::runtime_error);
    EXPECT_THROW((void)game::deserializeGameState("not valid protobuf data!@#$%"), std::runtime_error);
}

TEST(SerializationTest, BuildingYieldAndCostRoundTrip) {
    game::GameState state(10, 10, 42);
    state.addBuilding(game::makeMine(2, 2));

    std::string data = game::serializeGameState(state);
    game::GameState restored = game::deserializeGameState(data);

    ASSERT_EQ(restored.buildings().size(), 1U);
    const game::Building &b = restored.buildings()[0];
    EXPECT_EQ(b.yieldPerTurn().production, 2);
    EXPECT_EQ(b.constructionCost().production, 10);
}
