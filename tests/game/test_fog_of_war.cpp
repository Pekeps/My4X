// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/FogOfWar.h"

#include "game/City.h"
#include "game/Faction.h"
#include "game/Map.h"
#include "game/Unit.h"
#include "game/UnitTemplate.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

constexpr game::FactionId FACTION_A = 1;
constexpr game::FactionId FACTION_B = 2;
constexpr int MAP_ROWS = 10;
constexpr int MAP_COLS = 10;

/// Create a simple UnitTemplate with the given sight range.
game::UnitTemplate makeTemplate(int sightRange) {
    return game::UnitTemplate{
        .name = "Scout",
        .maxHealth = 100,
        .attack = 10,
        .defense = 5,
        .movementRange = 2,
        .attackRange = 1,
        .sightRange = sightRange,
        .productionCost = game::Resource{.gold = 0, .production = 10, .food = 0},
    };
}

} // namespace

// ── Default state: all tiles unexplored ─────────────────────────────────────

TEST(FogOfWarTest, DefaultUnexplored) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            EXPECT_EQ(fog.getVisibility(FACTION_A, r, c), game::VisibilityState::Unexplored)
                << "Tile (" << r << ", " << c << ") should default to Unexplored";
        }
    }
}

TEST(FogOfWarTest, UnknownFactionReturnsUnexplored) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    constexpr game::FactionId UNKNOWN = 99;
    EXPECT_EQ(fog.getVisibility(UNKNOWN, 0, 0), game::VisibilityState::Unexplored);
}

// ── Visibility from unit ────────────────────────────────────────────────────

TEST(FogOfWarTest, UnitMakesTilesVisible) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);

    auto tmpl = makeTemplate(2);
    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl, FACTION_A));

    std::vector<game::City> cities;

    fog.recalculate(FACTION_A, units, cities, map);

    // The unit's own tile must be Visible.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Visible);

    // Tiles within sight range should be Visible.
    // Hex distance of 1 from (5,5): immediate neighbors.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 4, 5), game::VisibilityState::Visible);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 6, 5), game::VisibilityState::Visible);

    // Tiles at hex distance 2 should also be Visible (sight range = 2).
    EXPECT_EQ(fog.getVisibility(FACTION_A, 3, 5), game::VisibilityState::Visible);
}

TEST(FogOfWarTest, TilesBeyondSightRangeRemainUnexplored) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);

    auto tmpl = makeTemplate(1);
    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl, FACTION_A));

    std::vector<game::City> cities;

    fog.recalculate(FACTION_A, units, cities, map);

    // (5,5) and immediate neighbors visible, but far corners should not be.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 0, 0), game::VisibilityState::Unexplored);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 9, 9), game::VisibilityState::Unexplored);
}

TEST(FogOfWarTest, UnitSightRangeFromTemplate) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);

    // Sight range 3 should see further than sight range 1.
    auto tmpl3 = makeTemplate(3);
    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl3, FACTION_A));

    std::vector<game::City> cities;

    fog.recalculate(FACTION_A, units, cities, map);

    // Tile at hex distance 3 from (5,5) should be Visible.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 2, 5), game::VisibilityState::Visible);
}

// ── Visibility from city ────────────────────────────────────────────────────

TEST(FogOfWarTest, CityMakesTilesVisible) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);

    std::vector<std::unique_ptr<game::Unit>> units;

    game::City city("TestCity", 5, 5, static_cast<int>(FACTION_A));
    city.addTile(5, 5);
    std::vector<game::City> cities;
    cities.push_back(std::move(city));

    fog.recalculate(FACTION_A, units, cities, map);

    // City center tile should be visible.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Visible);

    // Tiles within CITY_SIGHT_RANGE (3) should be Visible.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 4, 5), game::VisibilityState::Visible);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 3, 5), game::VisibilityState::Visible);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 2, 5), game::VisibilityState::Visible);
}

TEST(FogOfWarTest, CityVisionFromMultipleTiles) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);

    std::vector<std::unique_ptr<game::Unit>> units;

    game::City city("BigCity", 5, 5, static_cast<int>(FACTION_A));
    city.addTile(5, 5);
    city.addTile(5, 6);
    std::vector<game::City> cities;
    cities.push_back(std::move(city));

    fog.recalculate(FACTION_A, units, cities, map);

    // Both city tiles and their surroundings should be visible.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Visible);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 6), game::VisibilityState::Visible);
}

// ── Explored decay ──────────────────────────────────────────────────────────

TEST(FogOfWarTest, VisibleBecomesExploredWhenUnitMoves) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);
    std::vector<game::City> cities;

    auto tmpl = makeTemplate(1);

    // Unit at (5,5) — recalculate.
    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl, FACTION_A));
    fog.recalculate(FACTION_A, units, cities, map);

    // (5,5) should be Visible.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Visible);

    // Move unit away to (0,0) — recalculate.
    units.clear();
    units.push_back(std::make_unique<game::Unit>(0, 0, tmpl, FACTION_A));
    fog.recalculate(FACTION_A, units, cities, map);

    // (5,5) should now be Explored (was visible, no longer in range).
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Explored);

    // (0,0) should now be Visible.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 0, 0), game::VisibilityState::Visible);
}

TEST(FogOfWarTest, ExploredStaysExploredAcrossRecalculations) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);
    std::vector<game::City> cities;

    auto tmpl = makeTemplate(1);

    // Unit at (5,5).
    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl, FACTION_A));
    fog.recalculate(FACTION_A, units, cities, map);

    // Move to (0,0).
    units.clear();
    units.push_back(std::make_unique<game::Unit>(0, 0, tmpl, FACTION_A));
    fog.recalculate(FACTION_A, units, cities, map);

    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Explored);

    // Recalculate again with unit still at (0,0).
    fog.recalculate(FACTION_A, units, cities, map);

    // (5,5) should still be Explored, not reset to Unexplored.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Explored);
}

TEST(FogOfWarTest, ExploredBecomesVisibleWhenReentered) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);
    std::vector<game::City> cities;

    auto tmpl = makeTemplate(1);

    // Unit at (5,5).
    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl, FACTION_A));
    fog.recalculate(FACTION_A, units, cities, map);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Visible);

    // Move away.
    units.clear();
    units.push_back(std::make_unique<game::Unit>(0, 0, tmpl, FACTION_A));
    fog.recalculate(FACTION_A, units, cities, map);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Explored);

    // Move back.
    units.clear();
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl, FACTION_A));
    fog.recalculate(FACTION_A, units, cities, map);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Visible);
}

// ── Per-faction independence ────────────────────────────────────────────────

TEST(FogOfWarTest, FactionVisibilityIsIndependent) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);
    std::vector<game::City> cities;

    auto tmpl = makeTemplate(1);

    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl, FACTION_A));
    units.push_back(std::make_unique<game::Unit>(0, 0, tmpl, FACTION_B));

    fog.recalculate(FACTION_A, units, cities, map);
    fog.recalculate(FACTION_B, units, cities, map);

    // Faction A sees (5,5) as Visible but (0,0) as Unexplored (enemy unit doesn't grant vision).
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Visible);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 0, 0), game::VisibilityState::Unexplored);

    // Faction B sees (0,0) as Visible but (5,5) as Unexplored.
    EXPECT_EQ(fog.getVisibility(FACTION_B, 0, 0), game::VisibilityState::Visible);
    EXPECT_EQ(fog.getVisibility(FACTION_B, 5, 5), game::VisibilityState::Unexplored);
}

// ── Enemy units do not grant vision ─────────────────────────────────────────

TEST(FogOfWarTest, EnemyUnitsDoNotGrantVision) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);
    std::vector<game::City> cities;

    auto tmpl = makeTemplate(2);

    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(5, 5, tmpl, FACTION_B));

    fog.recalculate(FACTION_A, units, cities, map);

    // Faction A has no units — should see nothing.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 5, 5), game::VisibilityState::Unexplored);
}

// ── Dimension accessors ─────────────────────────────────────────────────────

TEST(FogOfWarTest, DimensionAccessors) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    EXPECT_EQ(fog.rows(), MAP_ROWS);
    EXPECT_EQ(fog.cols(), MAP_COLS);
}

// ── Hex distance utility ────────────────────────────────────────────────────

TEST(FogOfWarTest, HexDistanceSamePosition) {
    EXPECT_EQ(game::FogOfWar::hexDistance(5, 5, 5, 5), 0);
}

TEST(FogOfWarTest, HexDistanceAdjacent) {
    // Row 5 is odd: neighbors include (4,5), (4,6), (5,6), (6,6), (6,5), (5,4)
    EXPECT_EQ(game::FogOfWar::hexDistance(5, 5, 4, 5), 1);
    EXPECT_EQ(game::FogOfWar::hexDistance(5, 5, 5, 6), 1);
}

TEST(FogOfWarTest, HexDistanceTwo) {
    EXPECT_EQ(game::FogOfWar::hexDistance(5, 5, 3, 5), 2);
}

// ── Edge: units on map boundary ─────────────────────────────────────────────

TEST(FogOfWarTest, UnitOnMapEdge) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);
    std::vector<game::City> cities;

    auto tmpl = makeTemplate(2);

    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(0, 0, tmpl, FACTION_A));

    fog.recalculate(FACTION_A, units, cities, map);

    // The unit's own tile should be visible even at the edge.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 0, 0), game::VisibilityState::Visible);

    // Should not crash or go out of bounds. Far corner should be unexplored.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 9, 9), game::VisibilityState::Unexplored);
}

// ── No units or cities: nothing visible ─────────────────────────────────────

TEST(FogOfWarTest, NoUnitsNoCitiesNothingVisible) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);

    std::vector<std::unique_ptr<game::Unit>> units;
    std::vector<game::City> cities;

    fog.recalculate(FACTION_A, units, cities, map);

    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            EXPECT_EQ(fog.getVisibility(FACTION_A, r, c), game::VisibilityState::Unexplored);
        }
    }
}

// ── Multiple units combine vision ───────────────────────────────────────────

TEST(FogOfWarTest, MultipleUnitsCombineVision) {
    game::FogOfWar fog(MAP_ROWS, MAP_COLS);
    game::Map map(MAP_ROWS, MAP_COLS);
    std::vector<game::City> cities;

    auto tmpl = makeTemplate(1);

    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Unit>(2, 2, tmpl, FACTION_A));
    units.push_back(std::make_unique<game::Unit>(7, 7, tmpl, FACTION_A));

    fog.recalculate(FACTION_A, units, cities, map);

    // Both unit tiles should be visible.
    EXPECT_EQ(fog.getVisibility(FACTION_A, 2, 2), game::VisibilityState::Visible);
    EXPECT_EQ(fog.getVisibility(FACTION_A, 7, 7), game::VisibilityState::Visible);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
