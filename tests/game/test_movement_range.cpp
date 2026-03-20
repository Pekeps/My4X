// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/Pathfinding.h"

#include "game/Map.h"
#include "game/TerrainType.h"
#include "game/TileRegistry.h"
#include "game/Unit.h"
#include "game/UnitTemplate.h"
#include "game/UnitTypeRegistry.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace {

/// Helper: create a simple UnitTemplate for testing.
game::UnitTemplate makeTestTemplate() {
    game::UnitTemplate tmpl;
    tmpl.name = "TestUnit";
    tmpl.maxHealth = 100;
    tmpl.attack = 10;
    tmpl.defense = 10;
    tmpl.movementRange = 3;
    tmpl.attackRange = 1;
    return tmpl;
}

/// Helper: set all tiles to Plains for a clean baseline.
void setAllPlains(game::Map &map) {
    for (int r = 0; r < map.height(); ++r) {
        for (int c = 0; c < map.width(); ++c) {
            map.tile(r, c).setTerrainType(game::TerrainType::Plains);
        }
    }
}

/// Helper: check if a given tile is in the reachable set.
bool isReachable(const std::vector<game::ReachableTile> &tiles, int row, int col) {
    return std::ranges::any_of(tiles,
                               [row, col](const auto &t) { return t.coord.row == row && t.coord.col == col; });
}

/// Helper: find the cost to reach a specific tile, or -1 if not reachable.
int costTo(const std::vector<game::ReachableTile> &tiles, int row, int col) {
    for (const auto &t : tiles) {
        if (t.coord.row == row && t.coord.col == col) {
            return t.cost;
        }
    }
    return -1;
}

} // namespace

// ── Basic movement range tests ──────────────────────────────────────────────

TEST(MovementRangeTest, ZeroBudgetReturnsEmpty) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto result = game::computeMovementRange(5, 5, 0, map, registry, 1);
    EXPECT_TRUE(result.empty());
}

TEST(MovementRangeTest, StartTileNotIncluded) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto result = game::computeMovementRange(5, 5, 3, map, registry, 1);
    EXPECT_FALSE(isReachable(result, 5, 5));
}

TEST(MovementRangeTest, BudgetOneReachesAdjacentPlains) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto result = game::computeMovementRange(5, 5, 1, map, registry, 1);

    // Interior tile on a 10x10 map should have exactly 6 neighbors.
    auto neighbors = game::hexNeighbors(5, 5, 10, 10);
    EXPECT_EQ(neighbors.size(), 6);
    EXPECT_EQ(result.size(), 6);

    // All neighbors should be reachable with cost 1 on plains.
    for (const auto &n : neighbors) {
        EXPECT_TRUE(isReachable(result, n.row, n.col))
            << "Neighbor (" << n.row << "," << n.col << ") should be reachable";
        EXPECT_EQ(costTo(result, n.row, n.col), 1);
    }
}

TEST(MovementRangeTest, BudgetTwoReachesTwoRingsOnPlains) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto result = game::computeMovementRange(5, 5, 2, map, registry, 1);

    // Should reach all 6 immediate neighbors (cost 1).
    auto ring1 = game::hexNeighbors(5, 5, 10, 10);
    for (const auto &n : ring1) {
        EXPECT_TRUE(isReachable(result, n.row, n.col));
        EXPECT_EQ(costTo(result, n.row, n.col), 1);
    }

    // Should also reach tiles at distance 2 (cost 2).
    // Just check that we have more than 6 tiles.
    EXPECT_GT(result.size(), ring1.size());
}

// ── Terrain cost tests ──────────────────────────────────────────────────────

TEST(MovementRangeTest, HillsCostTwoMovement) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Make a neighbor tile hills (cost 2).
    map.tile(5, 6).setTerrainType(game::TerrainType::Hills);

    auto result = game::computeMovementRange(5, 5, 2, map, registry, 1);

    // Hills neighbor should be reachable with cost 2.
    EXPECT_TRUE(isReachable(result, 5, 6));
    EXPECT_EQ(costTo(result, 5, 6), 2);
}

TEST(MovementRangeTest, SwampCostThreeMovement) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Make a neighbor tile swamp (cost 3).
    map.tile(5, 6).setTerrainType(game::TerrainType::Swamp);

    // Budget 2: cannot reach swamp.
    auto result2 = game::computeMovementRange(5, 5, 2, map, registry, 1);
    EXPECT_FALSE(isReachable(result2, 5, 6));

    // Budget 3: can reach swamp.
    auto result3 = game::computeMovementRange(5, 5, 3, map, registry, 1);
    EXPECT_TRUE(isReachable(result3, 5, 6));
    EXPECT_EQ(costTo(result3, 5, 6), 3);
}

TEST(MovementRangeTest, TerrainCostsLimitRange) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Make all tiles in a line hills (cost 2 each).
    map.tile(5, 6).setTerrainType(game::TerrainType::Hills);
    map.tile(5, 7).setTerrainType(game::TerrainType::Hills);

    // Budget 3: can reach first hills (cost 2) but not second (cost 4).
    auto result = game::computeMovementRange(5, 5, 3, map, registry, 1);
    EXPECT_TRUE(isReachable(result, 5, 6));
    EXPECT_FALSE(isReachable(result, 5, 7));

    // Budget 4: can reach both.
    auto result4 = game::computeMovementRange(5, 5, 4, map, registry, 1);
    EXPECT_TRUE(isReachable(result4, 5, 6));
    EXPECT_TRUE(isReachable(result4, 5, 7));
}

// ── Impassable terrain tests ────────────────────────────────────────────────

TEST(MovementRangeTest, WaterIsImpassable) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    map.tile(5, 6).setTerrainType(game::TerrainType::Water);

    auto result = game::computeMovementRange(5, 5, 10, map, registry, 1);
    EXPECT_FALSE(isReachable(result, 5, 6));
}

TEST(MovementRangeTest, MountainIsImpassable) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    map.tile(5, 6).setTerrainType(game::TerrainType::Mountain);

    auto result = game::computeMovementRange(5, 5, 10, map, registry, 1);
    EXPECT_FALSE(isReachable(result, 5, 6));
}

TEST(MovementRangeTest, ImpassableBlocksButDoesNotPreventGoingAround) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Place water at (5,6) — blocking the direct east path.
    map.tile(5, 6).setTerrainType(game::TerrainType::Water);

    auto result = game::computeMovementRange(5, 5, 3, map, registry, 1);

    // Water tile itself is not reachable.
    EXPECT_FALSE(isReachable(result, 5, 6));

    // But tiles beyond the water (via other routes) may still be reachable.
    // (5,7) is 2 tiles east; it may be reachable via a detour depending on hex layout.
    // At minimum, all non-water adjacent tiles should be reachable.
    auto neighbors = game::hexNeighbors(5, 5, 10, 10);
    for (const auto &n : neighbors) {
        if (n.row == 5 && n.col == 6) {
            continue; // Water tile.
        }
        EXPECT_TRUE(isReachable(result, n.row, n.col))
            << "Non-water neighbor (" << n.row << "," << n.col << ") should be reachable";
    }
}

// ── Enemy unit blocking tests ───────────────────────────────────────────────

TEST(MovementRangeTest, EnemyUnitBlocksTile) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    game::FactionId friendlyFaction = 1;
    game::FactionId enemyFaction = 2;

    auto tmpl = makeTestTemplate();
    game::UnitTypeRegistry typeReg;
    typeReg.registerTemplate("TestUnit", tmpl);

    auto enemy = std::make_unique<game::Unit>(5, 6, typeReg.getTemplate("TestUnit"), enemyFaction);
    registry.registerUnit(5, 6, enemy.get());

    auto result = game::computeMovementRange(5, 5, 3, map, registry, friendlyFaction);
    EXPECT_FALSE(isReachable(result, 5, 6));
}

TEST(MovementRangeTest, FriendlyUnitDoesNotBlock) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    game::FactionId friendlyFaction = 1;

    auto tmpl = makeTestTemplate();
    game::UnitTypeRegistry typeReg;
    typeReg.registerTemplate("TestUnit", tmpl);

    auto friendly = std::make_unique<game::Unit>(5, 6, typeReg.getTemplate("TestUnit"), friendlyFaction);
    registry.registerUnit(5, 6, friendly.get());

    auto result = game::computeMovementRange(5, 5, 3, map, registry, friendlyFaction);
    EXPECT_TRUE(isReachable(result, 5, 6));
}

// ── Edge cases ──────────────────────────────────────────────────────────────

TEST(MovementRangeTest, CornerTileHasFewerNeighbors) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto result = game::computeMovementRange(0, 0, 1, map, registry, 1);
    // Corner tile should have fewer than 6 reachable neighbors.
    EXPECT_LT(result.size(), 6);
    EXPECT_GT(result.size(), 0);
}

TEST(MovementRangeTest, SurroundedByImpassableReturnsEmpty) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Surround (5,5) with water.
    auto neighbors = game::hexNeighbors(5, 5, 10, 10);
    for (const auto &n : neighbors) {
        map.tile(n.row, n.col).setTerrainType(game::TerrainType::Water);
    }

    auto result = game::computeMovementRange(5, 5, 10, map, registry, 1);
    EXPECT_TRUE(result.empty());
}

TEST(MovementRangeTest, LargeBudgetOnOpenMap) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // With budget 100 on all-plains 10x10 map, should reach all 99 other tiles.
    auto result = game::computeMovementRange(5, 5, 100, map, registry, 1);
    EXPECT_EQ(result.size(), 99); // 10*10 - 1 (start tile).
}

TEST(MovementRangeTest, CostsAreOptimal) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto result = game::computeMovementRange(5, 5, 5, map, registry, 1);

    // On all-plains, cost should equal hex distance.
    for (const auto &tile : result) {
        int dist = game::hexDistance(5, 5, tile.coord.row, tile.coord.col);
        EXPECT_EQ(tile.cost, dist)
            << "Tile (" << tile.coord.row << "," << tile.coord.col << ") cost " << tile.cost
            << " should equal hex distance " << dist;
    }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
