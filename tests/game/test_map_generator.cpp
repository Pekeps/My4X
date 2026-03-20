// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/MapGenerator.h"
#include "game/Map.h"
#include "game/TerrainType.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <set>

// ── Determinism ──────────────────────────────────────────────────────────────

TEST(MapGeneratorTest, SameSeedProducesSameMap) {
    game::MapGenerator gen1(30, 40, 4, 42);
    game::MapGenerator gen2(30, 40, 4, 42);

    auto map1 = gen1.generate();
    auto map2 = gen2.generate();

    for (int row = 0; row < map1.height(); ++row) {
        for (int col = 0; col < map1.width(); ++col) {
            EXPECT_EQ(map1.tile(row, col).terrainType(), map2.tile(row, col).terrainType())
                << "Mismatch at (" << row << ", " << col << ")";
        }
    }
}

TEST(MapGeneratorTest, DifferentSeedsProduceDifferentMaps) {
    game::MapGenerator gen1(30, 40, 4, 1);
    game::MapGenerator gen2(30, 40, 4, 9999);

    auto map1 = gen1.generate();
    auto map2 = gen2.generate();

    bool anyDifference = false;
    for (int row = 0; row < map1.height() && !anyDifference; ++row) {
        for (int col = 0; col < map1.width() && !anyDifference; ++col) {
            if (map1.tile(row, col).terrainType() != map2.tile(row, col).terrainType()) {
                anyDifference = true;
            }
        }
    }
    EXPECT_TRUE(anyDifference);
}

// ── Terrain variety ──────────────────────────────────────────────────────────

TEST(MapGeneratorTest, AllSevenTerrainTypesPresent) {
    // Use a large enough map that all terrain types should appear.
    game::MapGenerator gen(50, 50, 4, 12345);
    auto map = gen.generate();

    std::set<game::TerrainType> found;
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            found.insert(map.tile(row, col).terrainType());
        }
    }

    EXPECT_TRUE(found.count(game::TerrainType::Plains) > 0) << "Missing Plains";
    EXPECT_TRUE(found.count(game::TerrainType::Forest) > 0) << "Missing Forest";
    EXPECT_TRUE(found.count(game::TerrainType::Hills) > 0) << "Missing Hills";
    EXPECT_TRUE(found.count(game::TerrainType::Mountain) > 0) << "Missing Mountain";
    EXPECT_TRUE(found.count(game::TerrainType::Water) > 0) << "Missing Water";
    EXPECT_TRUE(found.count(game::TerrainType::Desert) > 0) << "Missing Desert";
    EXPECT_TRUE(found.count(game::TerrainType::Swamp) > 0) << "Missing Swamp";
    EXPECT_EQ(found.size(), game::TERRAIN_TYPE_COUNT);
}

// ── Terrain distribution sanity ──────────────────────────────────────────────

TEST(MapGeneratorTest, TerrainDistributionIsReasonable) {
    game::MapGenerator gen(60, 60, 4, 777);
    auto map = gen.generate();

    std::array<int, game::TERRAIN_TYPE_COUNT> counts = {};
    int total = map.height() * map.width();
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            ++counts[static_cast<std::size_t>(map.tile(row, col).terrainType())];
        }
    }

    // Each terrain type should have at least 1% of tiles.
    for (std::size_t i = 0; i < game::TERRAIN_TYPE_COUNT; ++i) {
        double pct = 100.0 * static_cast<double>(counts[i]) / static_cast<double>(total);
        EXPECT_GT(pct, 1.0) << "Terrain type " << i << " has only " << pct << "% of tiles";
    }

    // Plains should be the most common terrain (target ~40%).
    auto plainsIdx = static_cast<std::size_t>(game::TerrainType::Plains);
    double plainsPct =
        100.0 * static_cast<double>(counts[plainsIdx]) / static_cast<double>(total);
    EXPECT_GT(plainsPct, 15.0) << "Plains should be a significant portion of the map";
}

// ── Map validity ─────────────────────────────────────────────────────────────

TEST(MapGeneratorTest, GeneratedMapHasCorrectDimensions) {
    game::MapGenerator gen(25, 35, 2, 100);
    auto map = gen.generate();

    EXPECT_EQ(map.height(), 25);
    EXPECT_EQ(map.width(), 35);
}

TEST(MapGeneratorTest, AllTilesHaveValidCoordinates) {
    game::MapGenerator gen(20, 20, 2, 55);
    auto map = gen.generate();

    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            EXPECT_EQ(map.tile(row, col).row(), row);
            EXPECT_EQ(map.tile(row, col).col(), col);
        }
    }
}

// ── Spawn points ─────────────────────────────────────────────────────────────

TEST(MapGeneratorTest, SpawnPointsMatchFactionCount) {
    game::MapGenerator gen(40, 40, 4, 42);
    auto spawns = gen.spawnPoints();

    EXPECT_EQ(static_cast<int>(spawns.size()), 4);
}

TEST(MapGeneratorTest, SpawnPointsAreOnPassableTerrain) {
    game::MapGenerator gen(40, 40, 4, 42);
    auto map = gen.generate();
    auto spawns = gen.spawnPoints();

    for (const auto &[row, col] : spawns) {
        auto terrain = map.tile(row, col).terrainType();
        EXPECT_TRUE(game::getTerrainProperties(terrain).passable)
            << "Spawn at (" << row << ", " << col << ") is on impassable terrain "
            << static_cast<int>(terrain);
    }
}

TEST(MapGeneratorTest, SpawnPointsAreSpreadApart) {
    game::MapGenerator gen(40, 40, 4, 42);
    auto spawns = gen.spawnPoints();

    // Every pair of spawns should be at least MIN_SPAWN_DISTANCE apart.
    for (std::size_t i = 0; i < spawns.size(); ++i) {
        for (std::size_t j = i + 1; j < spawns.size(); ++j) {
            int dr = std::abs(spawns[i].first - spawns[j].first);
            int dc = std::abs(spawns[i].second - spawns[j].second);
            int chebyshev = std::max(dr, dc);
            EXPECT_GE(chebyshev, game::mapgen::MIN_SPAWN_DISTANCE)
                << "Spawns " << i << " and " << j << " are too close: " << chebyshev;
        }
    }
}

TEST(MapGeneratorTest, SpawnPointsDeterministic) {
    game::MapGenerator gen1(40, 40, 4, 42);
    game::MapGenerator gen2(40, 40, 4, 42);

    auto spawns1 = gen1.spawnPoints();
    auto spawns2 = gen2.spawnPoints();

    ASSERT_EQ(spawns1.size(), spawns2.size());
    for (std::size_t i = 0; i < spawns1.size(); ++i) {
        EXPECT_EQ(spawns1[i], spawns2[i]);
    }
}

// ── Edge cases ───────────────────────────────────────────────────────────────

TEST(MapGeneratorTest, SmallMapStillWorks) {
    game::MapGenerator gen(5, 5, 1, 99);
    auto map = gen.generate();

    EXPECT_EQ(map.height(), 5);
    EXPECT_EQ(map.width(), 5);
}

TEST(MapGeneratorTest, ZeroFactionsProducesNoSpawns) {
    game::MapGenerator gen(20, 20, 0, 42);
    auto spawns = gen.spawnPoints();
    EXPECT_TRUE(spawns.empty());
}

TEST(MapGeneratorTest, SingleFactionProducesOneSpawn) {
    game::MapGenerator gen(20, 20, 1, 42);
    auto spawns = gen.spawnPoints();
    EXPECT_EQ(spawns.size(), 1U);
}

TEST(MapGeneratorTest, MultipleGenerationsWithDifferentSeeds) {
    // Verify determinism across several seeds.
    for (std::uint64_t seed = 0; seed < 5; ++seed) {
        game::MapGenerator gen1(20, 20, 2, seed);
        game::MapGenerator gen2(20, 20, 2, seed);
        auto map1 = gen1.generate();
        auto map2 = gen2.generate();

        for (int row = 0; row < 20; ++row) {
            for (int col = 0; col < 20; ++col) {
                EXPECT_EQ(map1.tile(row, col).terrainType(),
                           map2.tile(row, col).terrainType());
            }
        }
    }
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
