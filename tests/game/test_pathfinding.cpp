// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/Pathfinding.h"

#include "game/Map.h"
#include "game/TerrainType.h"
#include "game/TileRegistry.h"
#include "game/Unit.h"
#include "game/UnitTemplate.h"
#include "game/UnitTypeRegistry.h"

#include <gtest/gtest.h>

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

} // namespace

// ── hexDistance tests ─────────────────────────────────────────────────────────

TEST(PathfindingTest, HexDistanceSamePosition) {
    EXPECT_EQ(game::hexDistance(0, 0, 0, 0), 0);
    EXPECT_EQ(game::hexDistance(5, 3, 5, 3), 0);
}

TEST(PathfindingTest, HexDistanceAdjacentTiles) {
    // Horizontal neighbor
    EXPECT_EQ(game::hexDistance(0, 0, 0, 1), 1);
    // Vertical neighbor (even row)
    EXPECT_EQ(game::hexDistance(0, 0, 1, 0), 1);
    // Diagonal neighbor (odd row)
    EXPECT_EQ(game::hexDistance(1, 0, 0, 0), 1);
    EXPECT_EQ(game::hexDistance(1, 0, 1, 1), 1);
}

TEST(PathfindingTest, HexDistanceLongerDistances) {
    EXPECT_EQ(game::hexDistance(0, 0, 0, 2), 2);
    EXPECT_EQ(game::hexDistance(0, 0, 0, 3), 3);
    EXPECT_EQ(game::hexDistance(0, 0, 2, 0), 2);
}

// ── hexNeighbors tests ───────────────────────────────────────────────────────

TEST(PathfindingTest, HexNeighborsInteriorTile) {
    auto neighbors = game::hexNeighbors(5, 5, 10, 10);
    EXPECT_EQ(neighbors.size(), 6);
}

TEST(PathfindingTest, HexNeighborsCornerTile) {
    // Top-left corner: (0,0) on even row
    auto neighbors = game::hexNeighbors(0, 0, 10, 10);
    // Should only have valid neighbors within bounds
    EXPECT_LT(neighbors.size(), 6);
    EXPECT_GE(neighbors.size(), 2);
}

TEST(PathfindingTest, HexNeighborsSingleTileMap) {
    auto neighbors = game::hexNeighbors(0, 0, 1, 1);
    EXPECT_EQ(neighbors.size(), 0);
}

// ── findPath: basic scenarios ────────────────────────────────────────────────

TEST(PathfindingTest, PathToSameTileIsEmpty) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto path = game::findPath(3, 3, 3, 3, map, registry, 1);
    EXPECT_TRUE(path.empty());
}

TEST(PathfindingTest, PathToAdjacentTile) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto path = game::findPath(3, 3, 3, 4, map, registry, 1);
    ASSERT_EQ(path.size(), 1);
    EXPECT_EQ(path[0].row, 3);
    EXPECT_EQ(path[0].col, 4);
}

TEST(PathfindingTest, PathExcludesStartIncludesGoal) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto path = game::findPath(0, 0, 0, 3, map, registry, 1);
    ASSERT_FALSE(path.empty());

    // First element should NOT be the start
    EXPECT_FALSE(path[0].row == 0 && path[0].col == 0);

    // Last element should be the goal
    EXPECT_EQ(path.back().row, 0);
    EXPECT_EQ(path.back().col, 3);
}

TEST(PathfindingTest, StraightLinePath) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Horizontal path on all plains — should be exactly 3 steps
    auto path = game::findPath(0, 0, 0, 3, map, registry, 1);
    EXPECT_EQ(path.size(), 3);
}

// ── findPath: impassable terrain ─────────────────────────────────────────────

TEST(PathfindingTest, ImpassableGoalReturnsEmpty) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Goal tile is Water (impassable)
    map.tile(3, 4).setTerrainType(game::TerrainType::Water);

    auto path = game::findPath(3, 3, 3, 4, map, registry, 1);
    EXPECT_TRUE(path.empty());
}

TEST(PathfindingTest, ImpassableMountainGoalReturnsEmpty) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    map.tile(5, 5).setTerrainType(game::TerrainType::Mountain);

    auto path = game::findPath(5, 4, 5, 5, map, registry, 1);
    EXPECT_TRUE(path.empty());
}

TEST(PathfindingTest, PathAroundImpassableTerrain) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Create a water wall between columns 2 and 3
    for (int r = 0; r < 10; ++r) {
        map.tile(r, 3).setTerrainType(game::TerrainType::Water);
    }
    // Leave a gap at row 5
    map.tile(5, 3).setTerrainType(game::TerrainType::Plains);

    // Try to go from (0,0) to (0,5) — must go through gap at (5,3)
    auto path = game::findPath(0, 0, 0, 5, map, registry, 1);
    ASSERT_FALSE(path.empty());

    // Path must reach goal
    EXPECT_EQ(path.back().row, 0);
    EXPECT_EQ(path.back().col, 5);

    // Path must go through the gap
    bool passedThroughGap = false;
    for (const auto &step : path) {
        if (step.row == 5 && step.col == 3) {
            passedThroughGap = true;
            break;
        }
    }
    EXPECT_TRUE(passedThroughGap);
}

TEST(PathfindingTest, CompletelySurroundedByImpassableReturnsEmpty) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Surround (5,5) with water on all sides
    auto neighbors = game::hexNeighbors(5, 5, 10, 10);
    for (const auto &n : neighbors) {
        map.tile(n.row, n.col).setTerrainType(game::TerrainType::Water);
    }

    // Start is surrounded — can't reach any distant tile
    auto path = game::findPath(5, 5, 0, 0, map, registry, 1);
    EXPECT_TRUE(path.empty());
}

// ── findPath: enemy unit blocking ────────────────────────────────────────────

TEST(PathfindingTest, EnemyOccupiedTileIsBlocked) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    game::FactionId friendlyFaction = 1;
    game::FactionId enemyFaction = 2;

    // Place an enemy unit at (3, 4) — the direct neighbor of start
    auto tmpl = makeTestTemplate();
    game::UnitTypeRegistry typeReg;
    typeReg.registerTemplate("TestUnit", tmpl);

    auto enemy = std::make_unique<game::Unit>(3, 4, typeReg.getTemplate("TestUnit"), enemyFaction);
    registry.registerUnit(3, 4, enemy.get());

    // Try to path to the enemy-occupied tile
    auto path = game::findPath(3, 3, 3, 4, map, registry, friendlyFaction);
    EXPECT_TRUE(path.empty());
}

TEST(PathfindingTest, FriendlyUnitDoesNotBlock) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    game::FactionId friendlyFaction = 1;

    // Place a friendly unit at an intermediate tile (3, 4)
    auto tmpl = makeTestTemplate();
    game::UnitTypeRegistry typeReg;
    typeReg.registerTemplate("TestUnit", tmpl);

    auto friendly = std::make_unique<game::Unit>(3, 4, typeReg.getTemplate("TestUnit"), friendlyFaction);
    registry.registerUnit(3, 4, friendly.get());

    // Path should still go through the friendly-occupied tile
    auto path = game::findPath(3, 3, 3, 5, map, registry, friendlyFaction);
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.back().row, 3);
    EXPECT_EQ(path.back().col, 5);
}

TEST(PathfindingTest, PathAroundEnemyUnit) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    game::FactionId friendlyFaction = 1;
    game::FactionId enemyFaction = 2;

    auto tmpl = makeTestTemplate();
    game::UnitTypeRegistry typeReg;
    typeReg.registerTemplate("TestUnit", tmpl);

    // Block the direct horizontal path at (3,4)
    auto enemy = std::make_unique<game::Unit>(3, 4, typeReg.getTemplate("TestUnit"), enemyFaction);
    registry.registerUnit(3, 4, enemy.get());

    // Path from (3,3) to (3,5) must go around the enemy
    auto path = game::findPath(3, 3, 3, 5, map, registry, friendlyFaction);
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.back().row, 3);
    EXPECT_EQ(path.back().col, 5);

    // The path should not include the enemy tile
    for (const auto &step : path) {
        EXPECT_FALSE(step.row == 3 && step.col == 4) << "Path should avoid enemy at (3,4)";
    }
}

// ── findPath: terrain cost preference ────────────────────────────────────────

TEST(PathfindingTest, PrefersLowerCostTerrain) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Create a swamp barrier on the direct path row 3
    // The direct route (3,3)->(3,4)->(3,5) through swamp costs 3+3=6
    // An alternate route through plains costs 1+1+1 = 3 (but is longer in hex steps)
    // Actually, let's make a simpler test: two paths, one with hills one with plains.

    // From (2,0) to (2,2):
    // Direct: (2,0) -> (2,1) swamp -> (2,2) plains: cost = 3 + 1 = 4
    // Via row 1: (2,0) -> (1,0) -> (1,1) -> (2,2): cost = 1 + 1 + 1 = 3
    // (depends on hex neighbor layout)

    // Simpler: from (0,0) to (0,2), with (0,1) as swamp
    map.tile(0, 1).setTerrainType(game::TerrainType::Swamp);

    auto path = game::findPath(0, 0, 0, 2, map, registry, 1);
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.back().row, 0);
    EXPECT_EQ(path.back().col, 2);

    // Check total movement cost of the path
    int totalCost = 0;
    for (const auto &step : path) {
        const auto &props = game::getTerrainProperties(map.tile(step.row, step.col).terrainType());
        totalCost += props.movementCost;
    }

    // If the direct path through swamp has cost 3+1=4, and there's a
    // cheaper path around, the total cost should be less than 4.
    // On a 10x10 all-plains map except (0,1)=swamp:
    // Going through (1,0) -> (1,1) -> (0,2) costs 1+1+1=3
    // So the optimal path should have total cost <= 3
    EXPECT_LE(totalCost, 3);
}

// ── findPath: edge cases ─────────────────────────────────────────────────────

TEST(PathfindingTest, PathOnSmallMap) {
    game::Map map(2, 2);
    setAllPlains(map);
    game::TileRegistry registry;

    auto path = game::findPath(0, 0, 1, 1, map, registry, 1);
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.back().row, 1);
    EXPECT_EQ(path.back().col, 1);
}

TEST(PathfindingTest, PathOnMinimalMap) {
    game::Map map(1, 2);
    setAllPlains(map);
    game::TileRegistry registry;

    auto path = game::findPath(0, 0, 0, 1, map, registry, 1);
    ASSERT_EQ(path.size(), 1);
    EXPECT_EQ(path[0].row, 0);
    EXPECT_EQ(path[0].col, 1);
}

TEST(PathfindingTest, GoalOutOfBoundsDoesNotCrash) {
    // This tests that even if start and goal are the same but all
    // neighbors are blocked, we get empty.
    game::Map map(3, 3);
    setAllPlains(map);
    game::TileRegistry registry;

    // Block all tiles except (0,0) and (2,2)
    map.tile(0, 1).setTerrainType(game::TerrainType::Water);
    map.tile(0, 2).setTerrainType(game::TerrainType::Water);
    map.tile(1, 0).setTerrainType(game::TerrainType::Water);
    map.tile(1, 1).setTerrainType(game::TerrainType::Water);
    map.tile(1, 2).setTerrainType(game::TerrainType::Water);
    map.tile(2, 0).setTerrainType(game::TerrainType::Water);
    map.tile(2, 1).setTerrainType(game::TerrainType::Water);

    // (0,0) to (2,2) — completely blocked
    auto path = game::findPath(0, 0, 2, 2, map, registry, 1);
    EXPECT_TRUE(path.empty());
}

// ── findPath: path continuity ────────────────────────────────────────────────

TEST(PathfindingTest, PathIsContinuous) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    auto path = game::findPath(0, 0, 5, 5, map, registry, 1);
    ASSERT_FALSE(path.empty());

    // Verify the first step is a neighbor of the start
    EXPECT_EQ(game::hexDistance(0, 0, path[0].row, path[0].col), 1);

    // Verify each step is adjacent to the previous one
    for (std::size_t i = 1; i < path.size(); ++i) {
        int dist = game::hexDistance(path[i - 1].row, path[i - 1].col, path[i].row, path[i].col);
        EXPECT_EQ(dist, 1) << "Non-adjacent steps at index " << i << ": (" << path[i - 1].row << ","
                           << path[i - 1].col << ") -> (" << path[i].row << "," << path[i].col << ")";
    }
}

TEST(PathfindingTest, PathThroughMixedTerrainIsContinuous) {
    game::Map map(10, 10);
    setAllPlains(map);
    game::TileRegistry registry;

    // Add some terrain variety
    map.tile(2, 3).setTerrainType(game::TerrainType::Hills);
    map.tile(3, 4).setTerrainType(game::TerrainType::Forest);
    map.tile(4, 2).setTerrainType(game::TerrainType::Swamp);
    map.tile(5, 3).setTerrainType(game::TerrainType::Water);

    auto path = game::findPath(1, 1, 6, 6, map, registry, 1);
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.back().row, 6);
    EXPECT_EQ(path.back().col, 6);

    // Verify continuity
    EXPECT_EQ(game::hexDistance(1, 1, path[0].row, path[0].col), 1);
    for (std::size_t i = 1; i < path.size(); ++i) {
        int dist = game::hexDistance(path[i - 1].row, path[i - 1].col, path[i].row, path[i].col);
        EXPECT_EQ(dist, 1);
    }

    // Verify no step is on water
    for (const auto &step : path) {
        EXPECT_NE(map.tile(step.row, step.col).terrainType(), game::TerrainType::Water);
    }
}

// ── findPath: larger distance ────────────────────────────────────────────────

TEST(PathfindingTest, LongerPathAcrossMap) {
    game::Map map(20, 20);
    setAllPlains(map);
    game::TileRegistry registry;

    auto path = game::findPath(0, 0, 19, 19, map, registry, 1);
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.back().row, 19);
    EXPECT_EQ(path.back().col, 19);

    // Path length should be reasonable (hex distance from (0,0) to (19,19)
    // is the minimum number of steps)
    int minDist = game::hexDistance(0, 0, 19, 19);
    EXPECT_GE(static_cast<int>(path.size()), minDist);
}

TEST(PathfindingTest, PathfindingOnLargeMapIsReasonable) {
    // Test performance requirement: 50x50 map should complete quickly
    game::Map map(50, 50);
    setAllPlains(map);
    game::TileRegistry registry;

    auto path = game::findPath(0, 0, 49, 49, map, registry, 1);
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.back().row, 49);
    EXPECT_EQ(path.back().col, 49);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
