// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/BuildQueue.h"
#include "game/Building.h"
#include "game/CaptureAction.h"
#include "game/City.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

#include <memory>

namespace {

/// Build a registry with the default unit types.
game::UnitTypeRegistry makeRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

/// Set up a basic game state with two factions at war and cities.
struct CaptureTestSetup {
    game::GameState state;
    game::FactionId factionA;
    game::FactionId factionB;

    CaptureTestSetup() : state(20, 20) {
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::NeutralHostile, 1);

        // Set factions at war.
        state.mutableDiplomacy().setRelation(factionA, factionB, game::DiplomacyState::War);
    }
};

} // namespace

// ── Validation tests ────────────────────────────────────────────────────────

TEST(CaptureActionTest, Validate_Valid_UndefendedEnemyCity) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // Create an enemy city at (5, 5).
    game::City enemyCity("EnemyTown", 5, 5, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(enemyCity));

    // Place a unit from factionA on the city center.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::Valid);
}

TEST(CaptureActionTest, Validate_UnitNotFound) {
    CaptureTestSetup setup;

    game::CaptureAction action(99);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::UnitNotFound);
}

TEST(CaptureActionTest, Validate_UnitDead) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.units()[0]->takeDamage(999);

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::UnitDead);
}

TEST(CaptureActionTest, Validate_NoCityAtPosition) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // Unit at position with no city.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::NoCityAtPosition);
}

TEST(CaptureActionTest, Validate_CityOwnedBySameFaction) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // Create a city owned by factionA.
    game::City ownCity("OwnCity", 5, 5, static_cast<int>(setup.factionA));
    setup.state.addCity(std::move(ownCity));

    // Place a unit from factionA on the city center.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::CityOwnedBySameFaction);
}

TEST(CaptureActionTest, Validate_NotAtWar) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // Set factions to peace.
    setup.state.mutableDiplomacy().setRelation(setup.factionA, setup.factionB, game::DiplomacyState::Peace);

    game::City enemyCity("PeacefulCity", 5, 5, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(enemyCity));

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::NotAtWar);
}

TEST(CaptureActionTest, Validate_CityIsDefended) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // Create an enemy city with tiles at (5,5) center and (5,6) extra.
    game::City enemyCity("DefendedCity", 5, 5, static_cast<int>(setup.factionB));
    enemyCity.addTile(5, 6);
    setup.state.addCity(std::move(enemyCity));

    // Place attacking unit on city center.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    // Place defending unit on city tile (5, 6).
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::CityIsDefended);
}

TEST(CaptureActionTest, Validate_DefenderOnCenterTileBlocksCapture) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // Create an enemy city.
    game::City enemyCity("CenterDefended", 5, 5, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(enemyCity));

    // Place attacking unit and defending unit both on center tile.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionB));

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::CityIsDefended);
}

TEST(CaptureActionTest, Validate_UnitNotOnCityCenter_OnlyOnCityTile) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // City center at (5, 5) with extra tile at (5, 6).
    game::City enemyCity("BigCity", 5, 5, static_cast<int>(setup.factionB));
    enemyCity.addTile(5, 6);
    setup.state.addCity(std::move(enemyCity));

    // Place unit on a city tile but NOT the center.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionA));

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::NoCityAtPosition);
}

// ── Execution tests ─────────────────────────────────────────────────────────

TEST(CaptureActionTest, Execute_OwnershipTransfers) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    game::City enemyCity("TargetCity", 5, 5, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(enemyCity));

    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_EQ(result.validation, game::CaptureValidation::Valid);
    EXPECT_EQ(result.capturedCityName, "TargetCity");
    EXPECT_EQ(result.previousOwner, setup.factionB);
    EXPECT_EQ(result.newOwner, setup.factionA);

    // Verify city ownership changed.
    auto *city = setup.state.findCity(1); // First city has ID 1
    ASSERT_NE(city, nullptr);
    EXPECT_EQ(static_cast<game::FactionId>(city->factionId()), setup.factionA);
}

TEST(CaptureActionTest, Execute_ProductionQueueCleared) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    game::City enemyCity("ProducingCity", 5, 5, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(enemyCity));

    // Enqueue some items in the city's build queue.
    auto *city = setup.state.findCity(1);
    ASSERT_NE(city, nullptr);
    city->buildQueue().enqueue(game::makeFarm, 5, 6);
    city->buildQueue().enqueueUnit("Warrior", 40);
    EXPECT_FALSE(city->buildQueue().isEmpty());

    // Place capturing unit.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);

    // Build queue should be cleared.
    city = setup.state.findCity(1);
    ASSERT_NE(city, nullptr);
    EXPECT_TRUE(city->buildQueue().isEmpty());
}

TEST(CaptureActionTest, Execute_BuildingsPreserved) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    game::City enemyCity("CityWithBuildings", 5, 5, static_cast<int>(setup.factionB));
    enemyCity.addTile(5, 6);
    setup.state.addCity(std::move(enemyCity));

    // Add buildings near the city.
    auto buildingsBefore = setup.state.buildings().size();
    setup.state.addBuilding(game::makeFarm(5, 6));
    EXPECT_EQ(setup.state.buildings().size(), buildingsBefore + 1);

    // Capture the city.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);

    // Buildings should still be there.
    EXPECT_EQ(setup.state.buildings().size(), buildingsBefore + 1);
}

TEST(CaptureActionTest, Execute_FailedValidation_NoExecution) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // No city at unit's position.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    auto result = action.execute(setup.state);

    EXPECT_FALSE(result.executed);
    EXPECT_EQ(result.validation, game::CaptureValidation::NoCityAtPosition);
}

// ── Faction elimination tests ───────────────────────────────────────────────

TEST(CaptureActionTest, FactionEliminated_NoCitiesNoUnits) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // FactionB has one city and no units.
    game::City enemyCity("LastCity", 5, 5, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(enemyCity));

    // FactionA captures it.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.factionEliminated);
    EXPECT_EQ(result.eliminatedFactionId, setup.factionB);
}

TEST(CaptureActionTest, FactionNotEliminated_HasOtherCity) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // FactionB has two cities.
    game::City city1("City1", 5, 5, static_cast<int>(setup.factionB));
    game::City city2("City2", 8, 8, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(city1));
    setup.state.addCity(std::move(city2));

    // Capture only the first city.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    game::CaptureAction action(0);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_FALSE(result.factionEliminated);
}

TEST(CaptureActionTest, FactionNotEliminated_HasUnitsRemaining) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // FactionB has one city and one unit elsewhere.
    game::City enemyCity("LastCity", 5, 5, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(enemyCity));
    setup.state.addUnit(std::make_unique<game::Warrior>(10, 10, reg, setup.factionB));

    // FactionA captures the city.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    // Unit at index 1 is factionA (factionB unit is at index 0).
    game::CaptureAction action(1);
    auto result = action.execute(setup.state);

    EXPECT_TRUE(result.executed);
    EXPECT_FALSE(result.factionEliminated); // FactionB still has a unit.
}

TEST(CaptureActionTest, IsFactionEliminated_Static) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // FactionB starts with nothing.
    EXPECT_TRUE(game::CaptureAction::isFactionEliminated(setup.state, setup.factionB));

    // Add a city for factionB.
    game::City city("City", 5, 5, static_cast<int>(setup.factionB));
    setup.state.addCity(std::move(city));
    EXPECT_FALSE(game::CaptureAction::isFactionEliminated(setup.state, setup.factionB));
}

TEST(CaptureActionTest, IsFactionEliminated_HasOnlyDeadUnits) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // FactionB has only dead units.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionB));
    setup.state.units()[0]->takeDamage(999);

    EXPECT_TRUE(game::CaptureAction::isFactionEliminated(setup.state, setup.factionB));
}

// ── BuildQueue::clear tests ─────────────────────────────────────────────────

TEST(BuildQueueTest, Clear_EmptiesQueue) {
    game::BuildQueue queue;
    queue.enqueue(game::makeFarm, 0, 0);
    queue.enqueueUnit("Warrior", 40);
    EXPECT_FALSE(queue.isEmpty());

    queue.clear();
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.accumulatedProduction(), 0);
}

TEST(BuildQueueTest, Clear_ResetsAccumulatedProduction) {
    game::BuildQueue queue;
    queue.enqueue(game::makeFarm, 0, 0);
    queue.applyProduction(10);
    EXPECT_GT(queue.accumulatedProduction(), 0);

    queue.clear();
    EXPECT_EQ(queue.accumulatedProduction(), 0);
}

TEST(BuildQueueTest, Clear_EmptyQueueIsNoop) {
    game::BuildQueue queue;
    EXPECT_TRUE(queue.isEmpty());

    queue.clear(); // Should not crash.
    EXPECT_TRUE(queue.isEmpty());
}

// ── Edge case: dead defending unit does not block capture ────────────────────

TEST(CaptureActionTest, DeadDefenderDoesNotBlockCapture) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    game::City enemyCity("Undefended", 5, 5, static_cast<int>(setup.factionB));
    enemyCity.addTile(5, 6);
    setup.state.addCity(std::move(enemyCity));

    // Place capturing unit at city center.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    // Place a dead defending unit on city tile.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, setup.factionB));
    setup.state.units()[1]->takeDamage(999);

    game::CaptureAction action(0);
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::Valid);
}

// ── Third-party unit on city tile does not block capture ─────────────────────

TEST(CaptureActionTest, ThirdPartyUnitOnCityTileDoesNotBlockCapture) {
    CaptureTestSetup setup;
    auto reg = makeRegistry();

    // Add a third faction.
    auto &registry = setup.state.mutableFactionRegistry();
    game::FactionId factionC = registry.addFaction("FactionC", game::FactionType::Player, 2);
    setup.state.mutableDiplomacy().setRelation(setup.factionA, factionC, game::DiplomacyState::War);
    setup.state.mutableDiplomacy().setRelation(setup.factionB, factionC, game::DiplomacyState::War);

    // Create enemy city.
    game::City enemyCity("DisputedCity", 5, 5, static_cast<int>(setup.factionB));
    enemyCity.addTile(5, 6);
    setup.state.addCity(std::move(enemyCity));

    // Place capturing unit on center.
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 5, reg, setup.factionA));

    // Place unit from third faction on city tile (not the defending faction).
    setup.state.addUnit(std::make_unique<game::Warrior>(5, 6, reg, factionC));

    game::CaptureAction action(0);
    // FactionC is not factionB, so city is not defended by its own faction.
    EXPECT_EQ(action.validate(setup.state), game::CaptureValidation::Valid);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
