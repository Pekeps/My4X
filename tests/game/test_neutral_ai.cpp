// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "game/NeutralAI.h"

#include "game/DiplomacyManager.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/TurnResolver.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <vector>

using namespace game;

// ── Helpers ──────────────────────────────────────────────────────────────────

static UnitTypeRegistry makeUnitRegistry() {
    UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

/// Build a minimal game state with one player, one hostile neutral,
/// and one passive neutral faction, with diplomacy initialized.
static GameState makeTestState() {
    GameState state(10, 10);
    auto &factions = state.mutableFactionRegistry();
    factions.addFaction("Player", FactionType::Player, 0);
    factions.addFaction("Barbarians", FactionType::NeutralHostile, 1);
    factions.addFaction("CityState", FactionType::NeutralPassive, 2);
    state.mutableDiplomacy().initializeDefaults(factions);
    return state;
}

static FactionId playerId(const GameState &state) {
    return state.factionRegistry().playerFactions()[0]->id();
}

static FactionId hostileId(const GameState &state) {
    for (const auto *f : state.factionRegistry().neutralFactions()) {
        if (f->type() == FactionType::NeutralHostile) {
            return f->id();
        }
    }
    return 0;
}

static FactionId passiveId(const GameState &state) {
    for (const auto *f : state.factionRegistry().neutralFactions()) {
        if (f->type() == FactionType::NeutralPassive) {
            return f->id();
        }
    }
    return 0;
}

// ── Spawning tests ───────────────────────────────────────────────────────────

TEST(NeutralAITest, SpawnCreatesHostileAndPassiveFactions) {
    GameState state(10, 10);
    NeutralAI::spawnNeutralFactions(state);

    auto neutrals = state.factionRegistry().neutralFactions();
    ASSERT_GE(neutrals.size(), 2);

    bool hasHostile = false;
    bool hasPassive = false;
    for (const auto *f : neutrals) {
        if (f->type() == FactionType::NeutralHostile) {
            hasHostile = true;
        }
        if (f->type() == FactionType::NeutralPassive) {
            hasPassive = true;
        }
    }
    EXPECT_TRUE(hasHostile);
    EXPECT_TRUE(hasPassive);
}

TEST(NeutralAITest, SpawnPlacesUnitsOnMap) {
    GameState state(10, 10);
    NeutralAI::spawnNeutralFactions(state);

    // Should have spawned units (3 hostile + 2 passive = 5).
    EXPECT_EQ(state.units().size(), 5);
}

TEST(NeutralAITest, SpawnedUnitsOwnedByNeutralFactions) {
    GameState state(10, 10);
    NeutralAI::spawnNeutralFactions(state);

    for (const auto &unit : state.units()) {
        const auto &faction = state.factionRegistry().getFaction(unit->factionId());
        EXPECT_NE(faction.type(), FactionType::Player);
    }
}

TEST(NeutralAITest, SpawnInitializesDiplomacy) {
    GameState state(10, 10);
    // Add a player faction first, then spawn neutrals.
    state.mutableFactionRegistry().addFaction("Player", FactionType::Player, 0);
    NeutralAI::spawnNeutralFactions(state);

    auto neutrals = state.factionRegistry().neutralFactions();
    FactionId hId = 0;
    FactionId pId = 0;
    for (const auto *f : neutrals) {
        if (f->type() == FactionType::NeutralHostile) {
            hId = f->id();
        }
        if (f->type() == FactionType::NeutralPassive) {
            pId = f->id();
        }
    }
    FactionId playerFid = state.factionRegistry().playerFactions()[0]->id();

    // Player vs Hostile: War
    EXPECT_TRUE(state.diplomacy().areAtWar(playerFid, hId));
    // Player vs Passive: Peace
    EXPECT_FALSE(state.diplomacy().areAtWar(playerFid, pId));
    // Hostile vs Passive: War
    EXPECT_TRUE(state.diplomacy().areAtWar(hId, pId));
}

TEST(NeutralAITest, SpawnClampsToMapBounds) {
    // Use a small map — some default positions may be out of bounds.
    GameState state(3, 3);
    NeutralAI::spawnNeutralFactions(state);

    // Only hostile positions (2,2) fit in a 3x3 map (valid: row<3, col<3).
    // Hostile: (2,2) fits, (2,3) does not, (3,2) does not.
    // Passive: (7,7) and (7,8) do not fit.
    EXPECT_EQ(state.units().size(), 1);
}

// ── Hold behavior tests ──────────────────────────────────────────────────────

TEST(NeutralAITest, HostileNeutralHoldsWhenNoEnemyAdjacent) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    state.addUnit(std::make_unique<Warrior>(5, 5, reg, hId));

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Hold);
    EXPECT_EQ(actions[0].targetRow, 5);
    EXPECT_EQ(actions[0].targetCol, 5);
}

TEST(NeutralAITest, PassiveNeutralHoldsWhenAtPeace) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId pId = passiveId(state);
    FactionId pFid = playerId(state);

    // Place passive unit and a player unit next to it.
    state.addUnit(std::make_unique<Warrior>(5, 5, reg, pId));
    state.addUnit(std::make_unique<Warrior>(5, 6, reg, pFid));

    // Passive and player are at peace by default.
    EXPECT_FALSE(state.diplomacy().areAtWar(pId, pFid));

    auto actions = NeutralAI::generateActions(state);

    // Only the passive unit generates actions (player units are not AI-controlled).
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Hold);
}

// ── Attack intent tests ──────────────────────────────────────────────────────

TEST(NeutralAITest, HostileNeutralAttacksAdjacentEnemy) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    FactionId pFid = playerId(state);

    // Place hostile unit at (4,4) and player unit at adjacent (4,5).
    state.addUnit(std::make_unique<Warrior>(4, 4, reg, hId));
    state.addUnit(std::make_unique<Warrior>(4, 5, reg, pFid));

    // Hostile vs Player should be at war.
    EXPECT_TRUE(state.diplomacy().areAtWar(hId, pFid));

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Attack);
    EXPECT_EQ(actions[0].targetRow, 4);
    EXPECT_EQ(actions[0].targetCol, 5);
    EXPECT_EQ(actions[0].actingFactionId, hId);
}

TEST(NeutralAITest, HostileDoesNotAttackAlliedUnit) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);

    // Add a second hostile faction.
    FactionId h2Id =
        state.mutableFactionRegistry().addFaction("Pirates", FactionType::NeutralHostile, 3);
    state.mutableDiplomacy().initializeDefaults(state.factionRegistry());

    // Hostile neutrals are at peace with each other.
    EXPECT_FALSE(state.diplomacy().areAtWar(hId, h2Id));

    state.addUnit(std::make_unique<Warrior>(4, 4, reg, hId));
    state.addUnit(std::make_unique<Warrior>(4, 5, reg, h2Id));

    auto actions = NeutralAI::generateActions(state);

    // Both factions generate actions. Neither should attack the other.
    for (const auto &action : actions) {
        EXPECT_EQ(action.type, AIActionType::Hold);
    }
}

TEST(NeutralAITest, PassiveNeutralAttacksAfterProvocation) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId pId = passiveId(state);
    FactionId pFid = playerId(state);

    // Place passive unit and player unit adjacent.
    state.addUnit(std::make_unique<Warrior>(4, 4, reg, pId));
    state.addUnit(std::make_unique<Warrior>(4, 5, reg, pFid));

    // Initially at peace — passive holds.
    auto actions1 = NeutralAI::generateActions(state);
    ASSERT_EQ(actions1.size(), 1);
    EXPECT_EQ(actions1[0].type, AIActionType::Hold);

    // Provoke: set the passive faction to war with the player.
    state.mutableDiplomacy().setRelation(pId, pFid, DiplomacyState::War);

    auto actions2 = NeutralAI::generateActions(state);
    ASSERT_EQ(actions2.size(), 1);
    EXPECT_EQ(actions2[0].type, AIActionType::Attack);
    EXPECT_EQ(actions2[0].targetRow, 4);
    EXPECT_EQ(actions2[0].targetCol, 5);
}

TEST(NeutralAITest, HostileDoesNotAttackDistantEnemy) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    FactionId pFid = playerId(state);

    // Place hostile at (2,2) and player far away at (8,8) — not adjacent.
    state.addUnit(std::make_unique<Warrior>(2, 2, reg, hId));
    state.addUnit(std::make_unique<Warrior>(8, 8, reg, pFid));

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Hold);
}

// ── Multiple unit tests ──────────────────────────────────────────────────────

TEST(NeutralAITest, MultipleHostileUnitsGenerateIndependentActions) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    FactionId pFid = playerId(state);

    // Two hostile units, one near a player, one not.
    state.addUnit(std::make_unique<Warrior>(4, 4, reg, hId));   // index 0
    state.addUnit(std::make_unique<Warrior>(4, 5, reg, pFid));  // index 1 (enemy)
    state.addUnit(std::make_unique<Warrior>(8, 8, reg, hId));   // index 2 (isolated)

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 2); // Two hostile units

    // Sort by unit index for deterministic checking.
    std::sort(actions.begin(), actions.end(),
              [](const AIAction &a, const AIAction &b) { return a.unitIndex < b.unitIndex; });

    EXPECT_EQ(actions[0].type, AIActionType::Attack);
    EXPECT_EQ(actions[0].unitIndex, 0);

    EXPECT_EQ(actions[1].type, AIActionType::Hold);
    EXPECT_EQ(actions[1].unitIndex, 2);
}

TEST(NeutralAITest, NoActionsWhenNoNeutralUnits) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId pFid = playerId(state);
    state.addUnit(std::make_unique<Warrior>(4, 4, reg, pFid));

    auto actions = NeutralAI::generateActions(state);
    EXPECT_TRUE(actions.empty());
}

TEST(NeutralAITest, DeadUnitsDoNotGenerateActions) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    state.addUnit(std::make_unique<Warrior>(4, 4, reg, hId));

    // Kill the unit.
    state.units()[0]->takeDamage(state.units()[0]->health());
    EXPECT_FALSE(state.units()[0]->isAlive());

    auto actions = NeutralAI::generateActions(state);
    EXPECT_TRUE(actions.empty());
}

// ── Hex neighbor adjacency tests ─────────────────────────────────────────────

TEST(NeutralAITest, HostileDetectsEnemyOnEvenRowHexNeighbor) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    FactionId pFid = playerId(state);

    // Place hostile at even row (4,4), player at (3,3) which is neighbor
    // for even row: (row-1, col-1).
    state.addUnit(std::make_unique<Warrior>(4, 4, reg, hId));
    state.addUnit(std::make_unique<Warrior>(3, 3, reg, pFid));

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Attack);
    EXPECT_EQ(actions[0].targetRow, 3);
    EXPECT_EQ(actions[0].targetCol, 3);
}

TEST(NeutralAITest, HostileDetectsEnemyOnOddRowHexNeighbor) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    FactionId pFid = playerId(state);

    // Place hostile at odd row (3,3), player at (2,4) which is neighbor
    // for odd row: (row-1, col+1).
    state.addUnit(std::make_unique<Warrior>(3, 3, reg, hId));
    state.addUnit(std::make_unique<Warrior>(2, 4, reg, pFid));

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Attack);
    EXPECT_EQ(actions[0].targetRow, 2);
    EXPECT_EQ(actions[0].targetCol, 4);
}

// ── Turn resolution integration ──────────────────────────────────────────────

TEST(NeutralAITest, TurnResolverGeneratesAIActions) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    FactionId pFid = playerId(state);

    state.addUnit(std::make_unique<Warrior>(4, 4, reg, hId));
    state.addUnit(std::make_unique<Warrior>(4, 5, reg, pFid));

    std::vector<AIAction> actions;
    resolveTurn(state, actions);

    // Should have generated one attack action for the hostile unit.
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Attack);
}

TEST(NeutralAITest, TurnResolverWithoutActionOutputStillWorks) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);
    state.addUnit(std::make_unique<Warrior>(4, 4, reg, hId));

    // The original resolveTurn(state) overload should not crash.
    EXPECT_EQ(state.getTurn(), 1);
    resolveTurn(state);
    EXPECT_EQ(state.getTurn(), 2);
}

// ── Edge cases ───────────────────────────────────────────────────────────────

TEST(NeutralAITest, UnitAtMapEdgeDoesNotCrash) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);

    // Place unit at corner (0,0).
    state.addUnit(std::make_unique<Warrior>(0, 0, reg, hId));

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Hold);
}

TEST(NeutralAITest, UnitAtBottomRightCornerDoesNotCrash) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId hId = hostileId(state);

    // Place unit at bottom-right corner (9,9).
    state.addUnit(std::make_unique<Warrior>(9, 9, reg, hId));

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, AIActionType::Hold);
}

TEST(NeutralAITest, EmptyStateProducesNoActions) {
    GameState state(10, 10);
    auto actions = NeutralAI::generateActions(state);
    EXPECT_TRUE(actions.empty());
}

TEST(NeutralAITest, ActionContainsCorrectUnitIndex) {
    auto state = makeTestState();
    auto reg = makeUnitRegistry();

    FactionId pFid = playerId(state);
    FactionId hId = hostileId(state);

    // Player unit at index 0, hostile at index 1.
    state.addUnit(std::make_unique<Warrior>(0, 0, reg, pFid));
    state.addUnit(std::make_unique<Warrior>(5, 5, reg, hId));

    auto actions = NeutralAI::generateActions(state);
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].unitIndex, 1);
    EXPECT_EQ(actions[0].actingFactionId, hId);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
