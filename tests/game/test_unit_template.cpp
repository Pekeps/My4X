// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/Unit.h"
#include "game/UnitTemplate.h"
#include "game/UnitTypeRegistry.h"

#include <gtest/gtest.h>
#include <memory>

namespace {

game::UnitTypeRegistry makeDefaultRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

} // namespace

// ---------------------------------------------------------------------------
// UnitTypeRegistry basics
// ---------------------------------------------------------------------------

TEST(UnitTypeRegistryTest, RegisterDefaultsPopulatesThreeTypes) {
    auto reg = makeDefaultRegistry();
    auto keys = reg.allKeys();
    EXPECT_EQ(keys.size(), 3U);
}

TEST(UnitTypeRegistryTest, HasTemplateReturnsTrue) {
    auto reg = makeDefaultRegistry();
    EXPECT_TRUE(reg.hasTemplate("Warrior"));
    EXPECT_TRUE(reg.hasTemplate("Archer"));
    EXPECT_TRUE(reg.hasTemplate("Settler"));
}

TEST(UnitTypeRegistryTest, HasTemplateReturnsFalse) {
    auto reg = makeDefaultRegistry();
    EXPECT_FALSE(reg.hasTemplate("Dragon"));
}

TEST(UnitTypeRegistryTest, GetTemplateThrowsForUnknown) {
    auto reg = makeDefaultRegistry();
    EXPECT_THROW((void)reg.getTemplate("Dragon"), std::out_of_range);
}

TEST(UnitTypeRegistryTest, DuplicateRegistrationThrows) {
    auto reg = makeDefaultRegistry();
    EXPECT_THROW(
        reg.registerTemplate("Warrior", game::UnitTemplate{.name = "Warrior", .productionCost = game::Resource{}}),
        std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Warrior template stats
// ---------------------------------------------------------------------------

TEST(UnitTypeRegistryTest, WarriorTemplateStats) {
    auto reg = makeDefaultRegistry();
    const auto &tmpl = reg.getTemplate("Warrior");
    EXPECT_EQ(tmpl.name, "Warrior");
    EXPECT_EQ(tmpl.maxHealth, 100);
    EXPECT_EQ(tmpl.attack, 15);
    EXPECT_EQ(tmpl.defense, 10);
    EXPECT_EQ(tmpl.movementRange, 2);
    EXPECT_EQ(tmpl.attackRange, 1);
    EXPECT_EQ(tmpl.productionCost.production, 20);
}

// ---------------------------------------------------------------------------
// Archer template stats
// ---------------------------------------------------------------------------

TEST(UnitTypeRegistryTest, ArcherTemplateStats) {
    auto reg = makeDefaultRegistry();
    const auto &tmpl = reg.getTemplate("Archer");
    EXPECT_EQ(tmpl.name, "Archer");
    EXPECT_EQ(tmpl.maxHealth, 70);
    EXPECT_EQ(tmpl.attack, 12);
    EXPECT_EQ(tmpl.defense, 5);
    EXPECT_EQ(tmpl.movementRange, 2);
    EXPECT_EQ(tmpl.attackRange, 3);
    EXPECT_EQ(tmpl.productionCost.production, 25);
}

// ---------------------------------------------------------------------------
// Settler template stats
// ---------------------------------------------------------------------------

TEST(UnitTypeRegistryTest, SettlerTemplateStats) {
    auto reg = makeDefaultRegistry();
    const auto &tmpl = reg.getTemplate("Settler");
    EXPECT_EQ(tmpl.name, "Settler");
    EXPECT_EQ(tmpl.maxHealth, 50);
    EXPECT_EQ(tmpl.attack, 0);
    EXPECT_EQ(tmpl.defense, 3);
    EXPECT_EQ(tmpl.movementRange, 3);
    EXPECT_EQ(tmpl.attackRange, 0);
    EXPECT_EQ(tmpl.productionCost.production, 40);
}

// ---------------------------------------------------------------------------
// Creating units directly from templates
// ---------------------------------------------------------------------------

TEST(UnitTemplateTest, CreateUnitFromWarriorTemplate) {
    auto reg = makeDefaultRegistry();
    const auto &tmpl = reg.getTemplate("Warrior");
    game::Unit unit(2, 3, tmpl);

    EXPECT_EQ(unit.name(), "Warrior");
    EXPECT_EQ(unit.health(), 100);
    EXPECT_EQ(unit.maxHealth(), 100);
    EXPECT_EQ(unit.movement(), 2);
    EXPECT_EQ(unit.attack(), 15);
    EXPECT_EQ(unit.defense(), 10);
    EXPECT_EQ(unit.attackRange(), 1);
    EXPECT_EQ(unit.row(), 2);
    EXPECT_EQ(unit.col(), 3);
}

TEST(UnitTemplateTest, CreateUnitFromArcherTemplate) {
    auto reg = makeDefaultRegistry();
    const auto &tmpl = reg.getTemplate("Archer");
    game::Unit unit(0, 0, tmpl);

    EXPECT_EQ(unit.name(), "Archer");
    EXPECT_EQ(unit.health(), 70);
    EXPECT_EQ(unit.maxHealth(), 70);
    EXPECT_EQ(unit.movement(), 2);
    EXPECT_EQ(unit.attack(), 12);
    EXPECT_EQ(unit.defense(), 5);
    EXPECT_EQ(unit.attackRange(), 3);
}

TEST(UnitTemplateTest, CreateUnitFromSettlerTemplate) {
    auto reg = makeDefaultRegistry();
    const auto &tmpl = reg.getTemplate("Settler");
    game::Unit unit(5, 5, tmpl);

    EXPECT_EQ(unit.name(), "Settler");
    EXPECT_EQ(unit.health(), 50);
    EXPECT_EQ(unit.maxHealth(), 50);
    EXPECT_EQ(unit.movement(), 3);
    EXPECT_EQ(unit.attack(), 0);
    EXPECT_EQ(unit.defense(), 3);
    EXPECT_EQ(unit.attackRange(), 0);
}

TEST(UnitTemplateTest, UnitReferencesTemplate) {
    auto reg = makeDefaultRegistry();
    const auto &tmpl = reg.getTemplate("Archer");
    game::Unit unit(0, 0, tmpl);

    EXPECT_EQ(&unit.unitTemplate(), &tmpl);
}

TEST(UnitTemplateTest, AllKeysAreSorted) {
    auto reg = makeDefaultRegistry();
    auto keys = reg.allKeys();
    ASSERT_EQ(keys.size(), 3U);
    EXPECT_EQ(keys[0], "Archer");
    EXPECT_EQ(keys[1], "Settler");
    EXPECT_EQ(keys[2], "Warrior");
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
