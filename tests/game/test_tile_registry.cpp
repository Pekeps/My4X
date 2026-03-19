#include "game/TileRegistry.h"
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

// --- City registration ---

TEST(TileRegistryTest, CityAtReturnsNulloptWhenEmpty) {
    game::TileRegistry registry;
    EXPECT_FALSE(registry.cityAt(0, 0).has_value());
}

TEST(TileRegistryTest, RegisterAndQueryCity) {
    game::TileRegistry registry;
    registry.registerCity(2, 3, 42);
    auto city = registry.cityAt(2, 3);
    ASSERT_TRUE(city.has_value());
    EXPECT_EQ(*city, 42);
}

TEST(TileRegistryTest, UnregisterCityRemovesIt) {
    game::TileRegistry registry;
    registry.registerCity(2, 3, 42);
    registry.unregisterCity(2, 3);
    EXPECT_FALSE(registry.cityAt(2, 3).has_value());
}

TEST(TileRegistryTest, RegisterCityOverwritesPrevious) {
    game::TileRegistry registry;
    registry.registerCity(1, 1, 10);
    registry.registerCity(1, 1, 20);
    auto city = registry.cityAt(1, 1);
    ASSERT_TRUE(city.has_value());
    EXPECT_EQ(*city, 20);
}

// --- Building registration ---

TEST(TileRegistryTest, BuildingAtReturnsNulloptWhenEmpty) {
    game::TileRegistry registry;
    EXPECT_FALSE(registry.buildingAt(0, 0).has_value());
}

TEST(TileRegistryTest, RegisterAndQueryBuilding) {
    game::TileRegistry registry;
    registry.registerBuilding(4, 5, 99);
    auto building = registry.buildingAt(4, 5);
    ASSERT_TRUE(building.has_value());
    EXPECT_EQ(*building, 99);
}

TEST(TileRegistryTest, UnregisterBuildingRemovesIt) {
    game::TileRegistry registry;
    registry.registerBuilding(4, 5, 99);
    registry.unregisterBuilding(4, 5);
    EXPECT_FALSE(registry.buildingAt(4, 5).has_value());
}

// --- Unit registration ---

TEST(TileRegistryTest, UnitsAtReturnsEmptyWhenNone) {
    game::TileRegistry registry;
    auto units = registry.unitsAt(0, 0);
    EXPECT_TRUE(units.empty());
}

TEST(TileRegistryTest, RegisterAndQueryUnit) {
    game::TileRegistry registry;
    auto unitReg = makeTestRegistry();
    game::Warrior warrior(1, 2, unitReg);
    registry.registerUnit(1, 2, &warrior);
    auto units = registry.unitsAt(1, 2);
    ASSERT_EQ(units.size(), 1);
    EXPECT_EQ(units[0], &warrior);
}

TEST(TileRegistryTest, MultipleUnitsOnSameTile) {
    game::TileRegistry registry;
    auto unitReg = makeTestRegistry();
    game::Warrior warrior1(3, 3, unitReg);
    game::Warrior warrior2(3, 3, unitReg);
    registry.registerUnit(3, 3, &warrior1);
    registry.registerUnit(3, 3, &warrior2);
    auto units = registry.unitsAt(3, 3);
    ASSERT_EQ(units.size(), 2);
    EXPECT_EQ(units[0], &warrior1);
    EXPECT_EQ(units[1], &warrior2);
}

TEST(TileRegistryTest, UnregisterUnitRemovesIt) {
    game::TileRegistry registry;
    auto unitReg = makeTestRegistry();
    game::Warrior warrior(1, 2, unitReg);
    registry.registerUnit(1, 2, &warrior);
    registry.unregisterUnit(1, 2, &warrior);
    auto units = registry.unitsAt(1, 2);
    EXPECT_TRUE(units.empty());
}

TEST(TileRegistryTest, UnregisterOneUnitLeavesOthers) {
    game::TileRegistry registry;
    auto unitReg = makeTestRegistry();
    game::Warrior warrior1(3, 3, unitReg);
    game::Warrior warrior2(3, 3, unitReg);
    registry.registerUnit(3, 3, &warrior1);
    registry.registerUnit(3, 3, &warrior2);
    registry.unregisterUnit(3, 3, &warrior1);
    auto units = registry.unitsAt(3, 3);
    ASSERT_EQ(units.size(), 1);
    EXPECT_EQ(units[0], &warrior2);
}

TEST(TileRegistryTest, UnregisterUnitFromEmptyTileIsNoOp) {
    game::TileRegistry registry;
    auto unitReg = makeTestRegistry();
    game::Warrior warrior(0, 0, unitReg);
    // Should not crash or throw.
    registry.unregisterUnit(0, 0, &warrior);
    EXPECT_TRUE(registry.unitsAt(0, 0).empty());
}

// --- isOccupied ---

TEST(TileRegistryTest, IsOccupiedReturnsFalseWhenEmpty) {
    game::TileRegistry registry;
    EXPECT_FALSE(registry.isOccupied(5, 5));
}

TEST(TileRegistryTest, IsOccupiedTrueWithCity) {
    game::TileRegistry registry;
    registry.registerCity(1, 1, 1);
    EXPECT_TRUE(registry.isOccupied(1, 1));
}

TEST(TileRegistryTest, IsOccupiedTrueWithBuilding) {
    game::TileRegistry registry;
    registry.registerBuilding(2, 2, 1);
    EXPECT_TRUE(registry.isOccupied(2, 2));
}

TEST(TileRegistryTest, IsOccupiedTrueWithUnit) {
    game::TileRegistry registry;
    auto unitReg = makeTestRegistry();
    game::Warrior warrior(4, 4, unitReg);
    registry.registerUnit(4, 4, &warrior);
    EXPECT_TRUE(registry.isOccupied(4, 4));
}

TEST(TileRegistryTest, IsOccupiedFalseAfterAllUnregistered) {
    game::TileRegistry registry;
    auto unitReg = makeTestRegistry();
    game::Warrior warrior(1, 1, unitReg);
    registry.registerCity(1, 1, 10);
    registry.registerBuilding(1, 1, 20);
    registry.registerUnit(1, 1, &warrior);

    registry.unregisterCity(1, 1);
    registry.unregisterBuilding(1, 1);
    registry.unregisterUnit(1, 1, &warrior);

    EXPECT_FALSE(registry.isOccupied(1, 1));
}

// --- Different tiles are independent ---

TEST(TileRegistryTest, DifferentTilesAreIndependent) {
    game::TileRegistry registry;
    registry.registerCity(0, 0, 1);
    registry.registerBuilding(1, 1, 2);

    EXPECT_TRUE(registry.cityAt(0, 0).has_value());
    EXPECT_FALSE(registry.cityAt(1, 1).has_value());
    EXPECT_FALSE(registry.buildingAt(0, 0).has_value());
    EXPECT_TRUE(registry.buildingAt(1, 1).has_value());
}
