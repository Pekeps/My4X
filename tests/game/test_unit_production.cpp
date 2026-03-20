// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "game/BuildQueue.h"
#include "game/City.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/TurnResolver.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"

#include <gtest/gtest.h>
#include <memory>
#include <variant>

using namespace game;

// ── Helpers ──────────────────────────────────────────────────────────────

static UnitTypeRegistry makeTestRegistry() {
    UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

static GameState makeTestState() { return GameState(10, 10); }

// ── BuildQueue unit order tests ──────────────────────────────────────────

TEST(UnitProductionTest, EnqueueUnitOrder) {
    BuildQueue bq;
    bq.enqueueUnit("Warrior", 20);
    EXPECT_FALSE(bq.isEmpty());
    auto item = bq.currentItem();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->name, "Warrior");
    EXPECT_EQ(item->productionCost, 20);
    EXPECT_EQ(item->orderType, BuildOrderType::Unit);
    EXPECT_EQ(item->templateKey, "Warrior");
}

TEST(UnitProductionTest, UnitOrderCompletesWithCorrectType) {
    BuildQueue bq;
    bq.enqueueUnit("Warrior", 20);
    auto result = bq.applyProduction(20);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<UnitSpawnRequest>(*result));
    EXPECT_EQ(std::get<UnitSpawnRequest>(*result).templateKey, "Warrior");
    EXPECT_TRUE(bq.isEmpty());
}

TEST(UnitProductionTest, UnitOrderPartialProduction) {
    BuildQueue bq;
    bq.enqueueUnit("Archer", 25);
    auto result = bq.applyProduction(10);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(bq.accumulatedProduction(), 10);
    EXPECT_FALSE(bq.isEmpty());
}

TEST(UnitProductionTest, MixedQueueBuildingThenUnit) {
    BuildQueue bq;
    bq.enqueue(makeFarm, 0, 0);       // Farm costs 20
    bq.enqueueUnit("Warrior", 20);    // Warrior costs 20

    // Complete the Farm
    auto farmResult = bq.applyProduction(20);
    ASSERT_TRUE(farmResult.has_value());
    ASSERT_TRUE(std::holds_alternative<Building>(*farmResult));
    EXPECT_EQ(std::get<Building>(*farmResult).name(), "Farm");

    // Now the Warrior order is front
    ASSERT_FALSE(bq.isEmpty());
    EXPECT_EQ(bq.currentItem()->name, "Warrior");
    EXPECT_EQ(bq.currentItem()->orderType, BuildOrderType::Unit);

    // Complete the Warrior
    auto warriorResult = bq.applyProduction(20);
    ASSERT_TRUE(warriorResult.has_value());
    ASSERT_TRUE(std::holds_alternative<UnitSpawnRequest>(*warriorResult));
    EXPECT_EQ(std::get<UnitSpawnRequest>(*warriorResult).templateKey, "Warrior");
    EXPECT_TRUE(bq.isEmpty());
}

TEST(UnitProductionTest, MixedQueueUnitThenBuilding) {
    BuildQueue bq;
    bq.enqueueUnit("Settler", 40);    // Settler costs 40
    bq.enqueue(makeMine, 1, 1);       // Mine costs 10

    // Complete the Settler
    auto settlerResult = bq.applyProduction(40);
    ASSERT_TRUE(settlerResult.has_value());
    ASSERT_TRUE(std::holds_alternative<UnitSpawnRequest>(*settlerResult));

    // Now the Mine order is front
    ASSERT_FALSE(bq.isEmpty());
    EXPECT_EQ(bq.currentItem()->name, "Mine");

    // Complete the Mine
    auto mineResult = bq.applyProduction(10);
    ASSERT_TRUE(mineResult.has_value());
    ASSERT_TRUE(std::holds_alternative<Building>(*mineResult));
    EXPECT_TRUE(bq.isEmpty());
}

TEST(UnitProductionTest, CancelUnitOrder) {
    BuildQueue bq;
    bq.enqueueUnit("Warrior", 20);
    bq.applyProduction(5);
    bq.cancel();
    EXPECT_TRUE(bq.isEmpty());
    EXPECT_EQ(bq.accumulatedProduction(), 0);
}

TEST(UnitProductionTest, TurnsRemainingForUnitOrder) {
    BuildQueue bq;
    // Warrior costs 20, 5 per turn => 4 turns
    bq.enqueueUnit("Warrior", 20);
    EXPECT_EQ(bq.turnsRemaining(5), 4);

    bq.applyProduction(5);
    // 15 remaining, 5 per turn => 3 turns
    EXPECT_EQ(bq.turnsRemaining(5), 3);
}

// ── TurnResolver integration tests ──────────────────────────────────────

TEST(UnitProductionTest, ResolveTurnSpawnsUnitOnCompletion) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    // Create faction and city
    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    City city("Rome", 3, 3, static_cast<int>(fid));
    state.addCity(std::move(city));

    // Enqueue Warrior (costs 20 production, base production = 5/turn => 4 turns)
    const auto &tmpl = reg.getTemplate("Warrior");
    state.mutableCities()[0].buildQueue().enqueueUnit("Warrior", tmpl.productionCost.production);

    // Resolve 3 turns (15 accumulated, not enough for 20)
    for (int i = 0; i < 3; ++i) {
        resolveTurn(state, &reg);
    }
    EXPECT_TRUE(state.units().empty());

    // 4th turn completes the warrior (20 accumulated)
    resolveTurn(state, &reg);
    ASSERT_EQ(state.units().size(), 1U);
    EXPECT_EQ(state.units()[0]->name(), "Warrior");
    EXPECT_EQ(state.units()[0]->factionId(), fid);
}

TEST(UnitProductionTest, SpawnedUnitHasCorrectStats) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    City city("Rome", 3, 3, static_cast<int>(fid));
    state.addCity(std::move(city));

    const auto &tmpl = reg.getTemplate("Archer");
    state.mutableCities()[0].buildQueue().enqueueUnit("Archer", tmpl.productionCost.production);

    // Complete production in one big batch (enough production)
    state.mutableCities()[0].buildQueue().setAccumulatedProduction(24);
    resolveTurn(state, &reg);

    ASSERT_EQ(state.units().size(), 1U);
    const auto &unit = *state.units()[0];
    EXPECT_EQ(unit.name(), "Archer");
    EXPECT_EQ(unit.maxHealth(), tmpl.maxHealth);
    EXPECT_EQ(unit.attack(), tmpl.attack);
    EXPECT_EQ(unit.defense(), tmpl.defense);
    EXPECT_EQ(unit.movement(), tmpl.movementRange);
    EXPECT_EQ(unit.factionId(), fid);
}

TEST(UnitProductionTest, UnitSpawnsAdjacentToCity) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    // City at center (4,4) so all neighbors are within the 10x10 map
    City city("Rome", 4, 4, static_cast<int>(fid));
    state.addCity(std::move(city));

    const auto &tmpl = reg.getTemplate("Warrior");
    state.mutableCities()[0].buildQueue().enqueueUnit("Warrior", tmpl.productionCost.production);

    // Speed up: pre-accumulate almost enough
    state.mutableCities()[0].buildQueue().setAccumulatedProduction(tmpl.productionCost.production - 1);
    resolveTurn(state, &reg);

    ASSERT_EQ(state.units().size(), 1U);
    int unitRow = state.units()[0]->row();
    int unitCol = state.units()[0]->col();

    // The unit should be on an adjacent tile (not on the city center)
    // or on the city center if all adjacent tiles are occupied.
    // For an empty map, it should be adjacent.
    int cityRow = 4;
    int cityCol = 4;
    int dr = unitRow - cityRow;
    int dc = unitCol - cityCol;
    // Adjacent means within 1 step in hex coordinates
    bool isAdjacent = (dr >= -1 && dr <= 1 && dc >= -1 && dc <= 1) && !(dr == 0 && dc == 0);
    bool isCityCenter = (unitRow == cityRow && unitCol == cityCol);
    EXPECT_TRUE(isAdjacent || isCityCenter);
}

TEST(UnitProductionTest, UnitSpawnsOnCityCenterWhenAllAdjacentOccupied) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    City city("Rome", 4, 4, static_cast<int>(fid));
    state.addCity(std::move(city));

    // Place units on all 6 adjacent tiles of (4,4) — even row
    // Even row (4) neighbors: (-1,-1),(-1,0),(0,1),(1,0),(1,-1),(0,-1)
    // => (3,3),(3,4),(4,5),(5,4),(5,3),(4,3)
    const auto &warriorTmpl = reg.getTemplate("Warrior");
    std::vector<std::pair<int, int>> neighbors = {{3, 3}, {3, 4}, {4, 5}, {5, 4}, {5, 3}, {4, 3}};
    for (const auto &[r, c] : neighbors) {
        auto blocker = std::make_unique<Unit>(r, c, warriorTmpl, fid);
        state.addUnit(std::move(blocker));
    }
    EXPECT_EQ(state.units().size(), 6U);

    // Now enqueue and complete a unit
    state.mutableCities()[0].buildQueue().enqueueUnit("Warrior", warriorTmpl.productionCost.production);
    state.mutableCities()[0].buildQueue().setAccumulatedProduction(warriorTmpl.productionCost.production - 1);
    resolveTurn(state, &reg);

    // Should have 7 units now
    ASSERT_EQ(state.units().size(), 7U);
    // The new unit should be at the city center since all adjacent tiles are occupied
    const auto &newUnit = *state.units()[6];
    EXPECT_EQ(newUnit.row(), 4);
    EXPECT_EQ(newUnit.col(), 4);
}

TEST(UnitProductionTest, UnitOwnershipMatchesCityFaction) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    FactionId fid1 = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    FactionId fid2 = state.mutableFactionRegistry().addFaction("Greece", FactionType::Player, 1);

    // Two cities from different factions
    City city1("Rome", 2, 2, static_cast<int>(fid1));
    state.addCity(std::move(city1));

    City city2("Athens", 7, 7, static_cast<int>(fid2));
    state.addCity(std::move(city2));

    const auto &tmpl = reg.getTemplate("Warrior");
    state.mutableCities()[0].buildQueue().enqueueUnit("Warrior", tmpl.productionCost.production);
    state.mutableCities()[0].buildQueue().setAccumulatedProduction(tmpl.productionCost.production - 1);

    state.mutableCities()[1].buildQueue().enqueueUnit("Warrior", tmpl.productionCost.production);
    state.mutableCities()[1].buildQueue().setAccumulatedProduction(tmpl.productionCost.production - 1);

    resolveTurn(state, &reg);

    ASSERT_EQ(state.units().size(), 2U);

    // Find which unit belongs to which faction
    bool foundRoman = false;
    bool foundGreek = false;
    for (const auto &unit : state.units()) {
        if (unit->factionId() == fid1) {
            foundRoman = true;
        }
        if (unit->factionId() == fid2) {
            foundGreek = true;
        }
    }
    EXPECT_TRUE(foundRoman);
    EXPECT_TRUE(foundGreek);
}

TEST(UnitProductionTest, ResourceCostDeductedWhenQueued) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);

    // Give the faction some resources
    state.mutableFactionRegistry().getMutableFaction(fid).stockpile().production = 100;
    state.mutableFactionRegistry().getMutableFaction(fid).stockpile().gold = 50;

    const auto &tmpl = reg.getTemplate("Warrior");
    Resource cost = tmpl.productionCost;

    // Deduct resource cost when queuing
    bool canAfford = state.mutableFactionRegistry().getMutableFaction(fid).spendResources(cost);
    EXPECT_TRUE(canAfford);

    // Now verify resources were deducted
    int expectedProduction = 100 - cost.production;
    EXPECT_EQ(state.factionRegistry().getFaction(fid).stockpile().production, expectedProduction);
}

TEST(UnitProductionTest, ResourceCostRejectedWhenInsufficientFunds) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    // Faction starts with 0 resources

    const auto &tmpl = reg.getTemplate("Warrior");
    Resource cost = tmpl.productionCost;

    bool canAfford = state.mutableFactionRegistry().getMutableFaction(fid).spendResources(cost);
    EXPECT_FALSE(canAfford);
    // Resources should be unchanged
    EXPECT_EQ(state.factionRegistry().getFaction(fid).stockpile().production, 0);
}

TEST(UnitProductionTest, BuildingProductionStillWorksWithUnitRegistry) {
    // Verify that passing a UnitTypeRegistry doesn't break building production
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    City city("TestCity", 1, 1);
    city.addTile(2, 2);
    state.addCity(std::move(city));

    state.mutableCities()[0].buildQueue().enqueue(makeMine, 3, 3);

    // Mine costs 10, base production = 5/turn => 2 turns
    resolveTurn(state, &reg);
    EXPECT_FALSE(state.mutableCities()[0].buildQueue().isEmpty());

    resolveTurn(state, &reg);
    EXPECT_TRUE(state.mutableCities()[0].buildQueue().isEmpty());

    bool found = false;
    for (const Building &b : state.buildings()) {
        if (b.name() == "Mine") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(UnitProductionTest, ResolveTurnWithoutRegistryIgnoresUnitOrders) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    City city("Rome", 3, 3, static_cast<int>(fid));
    state.addCity(std::move(city));

    const auto &tmpl = reg.getTemplate("Warrior");
    state.mutableCities()[0].buildQueue().enqueueUnit("Warrior", tmpl.productionCost.production);

    // Use the overload without a registry — unit orders complete but no unit spawns
    state.mutableCities()[0].buildQueue().setAccumulatedProduction(tmpl.productionCost.production - 1);
    resolveTurn(state);

    EXPECT_TRUE(state.units().empty());
    // Queue should be empty — order was consumed
    EXPECT_TRUE(state.mutableCities()[0].buildQueue().isEmpty());
}

TEST(UnitProductionTest, MultipleUnitsFromSameCity) {
    auto state = makeTestState();
    auto reg = makeTestRegistry();

    FactionId fid = state.mutableFactionRegistry().addFaction("Rome", FactionType::Player, 0);
    City city("Rome", 4, 4, static_cast<int>(fid));
    state.addCity(std::move(city));

    const auto &tmpl = reg.getTemplate("Warrior");

    // Enqueue two warriors
    state.mutableCities()[0].buildQueue().enqueueUnit("Warrior", tmpl.productionCost.production);
    state.mutableCities()[0].buildQueue().enqueueUnit("Warrior", tmpl.productionCost.production);

    // Complete first (20 production at 5/turn = 4 turns)
    for (int i = 0; i < 4; ++i) {
        resolveTurn(state, &reg);
    }
    EXPECT_EQ(state.units().size(), 1U);
    EXPECT_FALSE(state.mutableCities()[0].buildQueue().isEmpty());

    // Complete second
    for (int i = 0; i < 4; ++i) {
        resolveTurn(state, &reg);
    }
    EXPECT_EQ(state.units().size(), 2U);
    EXPECT_TRUE(state.mutableCities()[0].buildQueue().isEmpty());
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
