// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/Building.h"
#include "game/City.h"
#include "game/GamePersistence.h"
#include "game/GameState.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

namespace {

game::UnitTypeRegistry makeTestRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

} // namespace

TEST(GamePersistenceTest, SaveAndLoadRoundTrip) {
    game::GamePersistence db(":memory:");

    game::GameState state(8, 8, 42);
    for (int i = 0; i < 4; ++i) {
        state.nextTurn();
    }

    game::City city("Rome", 3, 4, 1);
    city.growPopulation(2);
    city.addTile(3, 5);
    state.addCity(std::move(city));

    state.addBuilding(game::makeFarm(5, 5));

    auto reg = makeTestRegistry();
    auto warrior = std::make_unique<game::Warrior>(7, 7, reg, 1);
    warrior->takeDamage(20);
    state.addUnit(std::move(warrior));

    state.factionResources().gold = 100;
    state.factionResources().production = 50;
    state.factionResources().food = 25;

    db.saveGame("game1", "Test Game", state);

    game::GameState loaded = db.loadGame("game1");

    EXPECT_EQ(loaded.getTurn(), 5);
    EXPECT_EQ(loaded.map().height(), 8);
    EXPECT_EQ(loaded.map().width(), 8);

    ASSERT_EQ(loaded.cities().size(), 1U);
    EXPECT_EQ(loaded.cities()[0].name(), "Rome");
    EXPECT_EQ(loaded.cities()[0].population(), 3);

    ASSERT_EQ(loaded.buildings().size(), 1U);
    EXPECT_EQ(loaded.buildings()[0].name(), "Farm");

    ASSERT_EQ(loaded.units().size(), 1U);
    EXPECT_EQ(loaded.units()[0]->name(), "Warrior");
    EXPECT_EQ(loaded.units()[0]->health(), 80);

    EXPECT_EQ(loaded.factionResources().gold, 100);
    EXPECT_EQ(loaded.factionResources().production, 50);
    EXPECT_EQ(loaded.factionResources().food, 25);
}

TEST(GamePersistenceTest, ListGames) {
    game::GamePersistence db(":memory:");

    game::GameState state1(5, 5, 42);
    db.saveGame("game-a", "Alpha", state1);

    game::GameState state2(5, 5, 42);
    state2.nextTurn();
    db.saveGame("game-b", "Beta", state2);

    auto games = db.listGames();
    ASSERT_EQ(games.size(), 2U);

    // Find each game by id (order not guaranteed within same second).
    const game::GameSummary *alpha = nullptr;
    const game::GameSummary *beta = nullptr;
    for (const auto &g : games) {
        if (g.id == "game-a") {
            alpha = &g;
        }
        if (g.id == "game-b") {
            beta = &g;
        }
    }

    ASSERT_NE(alpha, nullptr);
    EXPECT_EQ(alpha->name, "Alpha");
    EXPECT_EQ(alpha->turn, 1);
    EXPECT_FALSE(alpha->createdAt.empty());
    EXPECT_FALSE(alpha->updatedAt.empty());

    ASSERT_NE(beta, nullptr);
    EXPECT_EQ(beta->name, "Beta");
    EXPECT_EQ(beta->turn, 2);
    EXPECT_FALSE(beta->createdAt.empty());
    EXPECT_FALSE(beta->updatedAt.empty());
}

TEST(GamePersistenceTest, DeleteGame) {
    game::GamePersistence db(":memory:");

    game::GameState state(5, 5, 42);
    db.saveGame("game1", "To Delete", state);

    ASSERT_EQ(db.listGames().size(), 1U);

    db.deleteGame("game1");
    EXPECT_EQ(db.listGames().size(), 0U);

    // Loading a deleted game throws.
    EXPECT_THROW((void)db.loadGame("game1"), std::runtime_error);
}

TEST(GamePersistenceTest, DeleteNonexistentIsNoOp) {
    game::GamePersistence db(":memory:");

    // Should not throw.
    db.deleteGame("does-not-exist");
    EXPECT_EQ(db.listGames().size(), 0U);
}

TEST(GamePersistenceTest, OverwriteExistingGame) {
    game::GamePersistence db(":memory:");

    game::GameState state1(5, 5, 42);
    db.saveGame("game1", "Version 1", state1);

    game::GameState state2(8, 8, 99);
    for (int i = 0; i < 9; ++i) {
        state2.nextTurn();
    }
    db.saveGame("game1", "Version 2", state2);

    // Should still be one game.
    auto games = db.listGames();
    ASSERT_EQ(games.size(), 1U);
    EXPECT_EQ(games[0].name, "Version 2");
    EXPECT_EQ(games[0].turn, 10);

    // Load should return the new state.
    game::GameState loaded = db.loadGame("game1");
    EXPECT_EQ(loaded.getTurn(), 10);
    EXPECT_EQ(loaded.map().height(), 8);
    EXPECT_EQ(loaded.map().width(), 8);
}

TEST(GamePersistenceTest, LoadNonexistentThrows) {
    game::GamePersistence db(":memory:");
    EXPECT_THROW((void)db.loadGame("nonexistent"), std::runtime_error);
}

TEST(GamePersistenceTest, EmptyGameStateRoundTrip) {
    game::GamePersistence db(":memory:");

    game::GameState state(5, 5, 42);
    db.saveGame("empty", "Empty Game", state);

    game::GameState loaded = db.loadGame("empty");
    EXPECT_EQ(loaded.getTurn(), 1);
    EXPECT_EQ(loaded.map().height(), 5);
    EXPECT_EQ(loaded.map().width(), 5);
    EXPECT_EQ(loaded.cities().size(), 0U);
    EXPECT_EQ(loaded.buildings().size(), 0U);
    EXPECT_EQ(loaded.units().size(), 0U);
}

TEST(GamePersistenceTest, MapTerrainPreserved) {
    game::GamePersistence db(":memory:");

    game::GameState state(10, 10, 12345);
    db.saveGame("terrain", "Terrain Test", state);

    game::GameState loaded = db.loadGame("terrain");

    for (int row = 0; row < 10; ++row) {
        for (int col = 0; col < 10; ++col) {
            EXPECT_EQ(loaded.map().tile(row, col).terrainType(), state.map().tile(row, col).terrainType())
                << "Terrain mismatch at (" << row << ", " << col << ")";
        }
    }
}

TEST(GamePersistenceTest, MoveConstructor) {
    game::GamePersistence db1(":memory:");

    game::GameState state(5, 5, 42);
    db1.saveGame("game1", "Test", state);

    game::GamePersistence db2(std::move(db1));

    // db2 should work fine.
    game::GameState loaded = db2.loadGame("game1");
    EXPECT_EQ(loaded.getTurn(), 1);
}

TEST(GamePersistenceTest, PlayerCountTracked) {
    game::GamePersistence db(":memory:");

    game::GameState state(10, 10, 42);
    auto reg = makeTestRegistry();

    // Add units/cities from two different factions.
    game::City city1("Rome", 3, 4, 1);
    state.addCity(std::move(city1));
    game::City city2("Athens", 5, 5, 2);
    state.addCity(std::move(city2));

    state.addUnit(std::make_unique<game::Warrior>(7, 7, reg, 1));
    state.addUnit(std::make_unique<game::Warrior>(8, 8, reg, 2));

    db.saveGame("game1", "Multi-Player", state);

    auto games = db.listGames();
    ASSERT_EQ(games.size(), 1U);
    EXPECT_EQ(games[0].playerCount, 2);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
