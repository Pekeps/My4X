// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/CombatLog.h"

#include <gtest/gtest.h>

namespace {

/// Helper to create a CombatEvent with specific values for testing.
game::CombatEvent makeEvent(int turn, game::FactionId attackerFaction, game::FactionId defenderFaction,
                            int damageToDefender, int damageToAttacker, bool defenderDied = false,
                            bool attackerDied = false) {
    game::CombatEvent event;
    event.attackerUnitIndex = 0;
    event.defenderUnitIndex = 1;
    event.attackerFactionId = attackerFaction;
    event.defenderFactionId = defenderFaction;
    event.damageToDefender = damageToDefender;
    event.damageToAttacker = damageToAttacker;
    event.defenderDied = defenderDied;
    event.attackerDied = attackerDied;
    event.turn = turn;
    event.tileRow = 3;
    event.tileCol = 5;
    event.defenderTerrain = game::TerrainType::Hills;
    return event;
}

} // namespace

// ── Construction ─────────────────────────────────────────────────────────────

TEST(CombatLogTest, DefaultConstruction_EmptyWithDefaultCapacity) {
    game::CombatLog log;
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(log.size(), 0);
    EXPECT_EQ(log.capacity(), game::CombatLog::DEFAULT_CAPACITY);
}

TEST(CombatLogTest, CustomCapacity) {
    game::CombatLog log(50);
    EXPECT_EQ(log.capacity(), 50);
    EXPECT_TRUE(log.empty());
}

// ── Append and size ──────────────────────────────────────────────────────────

TEST(CombatLogTest, AppendSingle_SizeIncreases) {
    game::CombatLog log(10);
    log.append(makeEvent(1, 1, 2, 10, 5));
    EXPECT_EQ(log.size(), 1);
    EXPECT_FALSE(log.empty());
}

TEST(CombatLogTest, AppendMultiple_SizeMatchesCount) {
    game::CombatLog log(10);
    log.append(makeEvent(1, 1, 2, 10, 5));
    log.append(makeEvent(1, 1, 2, 8, 3));
    log.append(makeEvent(2, 2, 1, 12, 0, true));
    EXPECT_EQ(log.size(), 3);
}

// ── Capacity and ring buffer ─────────────────────────────────────────────────

TEST(CombatLogTest, ExceedCapacity_OldestDropped) {
    game::CombatLog log(3);

    // Fill to capacity
    log.append(makeEvent(1, 1, 2, 10, 0)); // event A (oldest)
    log.append(makeEvent(1, 1, 2, 20, 0)); // event B
    log.append(makeEvent(2, 1, 2, 30, 0)); // event C

    EXPECT_EQ(log.size(), 3);

    // Overflow: event A should be dropped
    log.append(makeEvent(2, 1, 2, 40, 0)); // event D replaces A
    EXPECT_EQ(log.size(), 3);

    auto all = log.allEvents();
    ASSERT_EQ(all.size(), 3);
    // Should be B, C, D (A was dropped)
    EXPECT_EQ(all[0].damageToDefender, 20);
    EXPECT_EQ(all[1].damageToDefender, 30);
    EXPECT_EQ(all[2].damageToDefender, 40);
}

TEST(CombatLogTest, DoubleOverflow_CorrectOrder) {
    game::CombatLog log(3);

    // Insert 6 events into a size-3 buffer
    for (int i = 1; i <= 6; ++i) {
        log.append(makeEvent(i, 1, 2, i * 10, 0));
    }

    EXPECT_EQ(log.size(), 3);

    auto all = log.allEvents();
    ASSERT_EQ(all.size(), 3);
    // Only the last 3 should remain: events 4, 5, 6
    EXPECT_EQ(all[0].damageToDefender, 40);
    EXPECT_EQ(all[1].damageToDefender, 50);
    EXPECT_EQ(all[2].damageToDefender, 60);
}

// ── RecentEvents ─────────────────────────────────────────────────────────────

TEST(CombatLogTest, RecentEvents_ReturnsRequestedCount) {
    game::CombatLog log(10);
    for (int i = 1; i <= 5; ++i) {
        log.append(makeEvent(i, 1, 2, i * 10, 0));
    }

    auto recent = log.recentEvents(3);
    ASSERT_EQ(recent.size(), 3);
    // Should be events 3, 4, 5 (oldest to newest of the last 3)
    EXPECT_EQ(recent[0].damageToDefender, 30);
    EXPECT_EQ(recent[1].damageToDefender, 40);
    EXPECT_EQ(recent[2].damageToDefender, 50);
}

TEST(CombatLogTest, RecentEvents_RequestMoreThanAvailable) {
    game::CombatLog log(10);
    log.append(makeEvent(1, 1, 2, 10, 0));
    log.append(makeEvent(1, 1, 2, 20, 0));

    auto recent = log.recentEvents(100);
    ASSERT_EQ(recent.size(), 2);
    EXPECT_EQ(recent[0].damageToDefender, 10);
    EXPECT_EQ(recent[1].damageToDefender, 20);
}

TEST(CombatLogTest, RecentEvents_EmptyLog) {
    game::CombatLog log(10);
    auto recent = log.recentEvents(5);
    EXPECT_TRUE(recent.empty());
}

TEST(CombatLogTest, RecentEvents_AfterOverflow) {
    game::CombatLog log(3);
    for (int i = 1; i <= 5; ++i) {
        log.append(makeEvent(i, 1, 2, i * 10, 0));
    }

    // Buffer has events 3, 4, 5
    auto recent = log.recentEvents(2);
    ASSERT_EQ(recent.size(), 2);
    EXPECT_EQ(recent[0].damageToDefender, 40);
    EXPECT_EQ(recent[1].damageToDefender, 50);
}

// ── EventsForTurn ────────────────────────────────────────────────────────────

TEST(CombatLogTest, EventsForTurn_FiltersByTurn) {
    game::CombatLog log(10);
    log.append(makeEvent(1, 1, 2, 10, 0));
    log.append(makeEvent(2, 1, 2, 20, 0));
    log.append(makeEvent(1, 2, 1, 30, 0));
    log.append(makeEvent(3, 1, 2, 40, 0));
    log.append(makeEvent(2, 2, 1, 50, 0));

    auto turn1 = log.eventsForTurn(1);
    ASSERT_EQ(turn1.size(), 2);
    EXPECT_EQ(turn1[0].damageToDefender, 10);
    EXPECT_EQ(turn1[1].damageToDefender, 30);

    auto turn2 = log.eventsForTurn(2);
    ASSERT_EQ(turn2.size(), 2);
    EXPECT_EQ(turn2[0].damageToDefender, 20);
    EXPECT_EQ(turn2[1].damageToDefender, 50);

    auto turn3 = log.eventsForTurn(3);
    ASSERT_EQ(turn3.size(), 1);
    EXPECT_EQ(turn3[0].damageToDefender, 40);
}

TEST(CombatLogTest, EventsForTurn_NoMatch) {
    game::CombatLog log(10);
    log.append(makeEvent(1, 1, 2, 10, 0));

    auto result = log.eventsForTurn(99);
    EXPECT_TRUE(result.empty());
}

TEST(CombatLogTest, EventsForTurn_EmptyLog) {
    game::CombatLog log(10);
    auto result = log.eventsForTurn(1);
    EXPECT_TRUE(result.empty());
}

TEST(CombatLogTest, EventsForTurn_AfterOverflow) {
    game::CombatLog log(3);
    // Turn 1 events get evicted as more are added
    log.append(makeEvent(1, 1, 2, 10, 0));
    log.append(makeEvent(1, 1, 2, 20, 0));
    log.append(makeEvent(2, 1, 2, 30, 0));
    log.append(makeEvent(2, 1, 2, 40, 0)); // evicts first turn-1 event
    log.append(makeEvent(3, 1, 2, 50, 0)); // evicts second turn-1 event

    auto turn1 = log.eventsForTurn(1);
    EXPECT_TRUE(turn1.empty()); // all turn-1 events were evicted

    auto turn2 = log.eventsForTurn(2);
    ASSERT_EQ(turn2.size(), 2);
    EXPECT_EQ(turn2[0].damageToDefender, 30);
    EXPECT_EQ(turn2[1].damageToDefender, 40);
}

// ── AllEvents ────────────────────────────────────────────────────────────────

TEST(CombatLogTest, AllEvents_ReturnsEverythingInOrder) {
    game::CombatLog log(10);
    log.append(makeEvent(1, 1, 2, 10, 0));
    log.append(makeEvent(1, 1, 2, 20, 0));
    log.append(makeEvent(2, 1, 2, 30, 0));

    auto all = log.allEvents();
    ASSERT_EQ(all.size(), 3);
    EXPECT_EQ(all[0].damageToDefender, 10);
    EXPECT_EQ(all[1].damageToDefender, 20);
    EXPECT_EQ(all[2].damageToDefender, 30);
}

TEST(CombatLogTest, AllEvents_EmptyLog) {
    game::CombatLog log(10);
    auto all = log.allEvents();
    EXPECT_TRUE(all.empty());
}

// ── Clear ────────────────────────────────────────────────────────────────────

TEST(CombatLogTest, Clear_ResetsLog) {
    game::CombatLog log(10);
    log.append(makeEvent(1, 1, 2, 10, 0));
    log.append(makeEvent(1, 1, 2, 20, 0));

    log.clear();

    EXPECT_TRUE(log.empty());
    EXPECT_EQ(log.size(), 0);
    EXPECT_TRUE(log.allEvents().empty());
}

TEST(CombatLogTest, Clear_CanAppendAfterClear) {
    game::CombatLog log(10);
    log.append(makeEvent(1, 1, 2, 10, 0));
    log.clear();

    log.append(makeEvent(2, 1, 2, 99, 0));
    EXPECT_EQ(log.size(), 1);

    auto all = log.allEvents();
    ASSERT_EQ(all.size(), 1);
    EXPECT_EQ(all[0].damageToDefender, 99);
}

// ── CombatEvent fields ───────────────────────────────────────────────────────

TEST(CombatLogTest, EventFieldsPreserved) {
    game::CombatLog log(10);

    game::CombatEvent event;
    event.attackerUnitIndex = 42;
    event.defenderUnitIndex = 7;
    event.attackerFactionId = 3;
    event.defenderFactionId = 5;
    event.damageToDefender = 15;
    event.damageToAttacker = 8;
    event.defenderDied = true;
    event.attackerDied = false;
    event.turn = 12;
    event.tileRow = 4;
    event.tileCol = 9;
    event.defenderTerrain = game::TerrainType::Forest;

    log.append(event);

    auto all = log.allEvents();
    ASSERT_EQ(all.size(), 1);
    const auto &stored = all[0];
    EXPECT_EQ(stored.attackerUnitIndex, 42);
    EXPECT_EQ(stored.defenderUnitIndex, 7);
    EXPECT_EQ(stored.attackerFactionId, 3);
    EXPECT_EQ(stored.defenderFactionId, 5);
    EXPECT_EQ(stored.damageToDefender, 15);
    EXPECT_EQ(stored.damageToAttacker, 8);
    EXPECT_TRUE(stored.defenderDied);
    EXPECT_FALSE(stored.attackerDied);
    EXPECT_EQ(stored.turn, 12);
    EXPECT_EQ(stored.tileRow, 4);
    EXPECT_EQ(stored.tileCol, 9);
    EXPECT_EQ(stored.defenderTerrain, game::TerrainType::Forest);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
