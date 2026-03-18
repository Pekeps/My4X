#include "game/BuildQueue.h"
#include <gtest/gtest.h>

using namespace game;

TEST(BuildQueueTest, IsEmptyInitially) {
    BuildQueue bq;
    EXPECT_TRUE(bq.isEmpty());
    EXPECT_FALSE(bq.currentItem().has_value());
    EXPECT_EQ(bq.accumulatedProduction(), 0);
}

TEST(BuildQueueTest, EnqueueMakesNonEmpty) {
    BuildQueue bq;
    bq.enqueue(makeFarm, 1, 2);
    EXPECT_FALSE(bq.isEmpty());
    ASSERT_TRUE(bq.currentItem().has_value());
    EXPECT_EQ(bq.currentItem()->name, "Farm");
    EXPECT_EQ(bq.currentItem()->targetRow, 1);
    EXPECT_EQ(bq.currentItem()->targetCol, 2);
}

TEST(BuildQueueTest, ApplyProductionPartialDoesNotComplete) {
    BuildQueue bq;
    // Farm costs 20 production
    bq.enqueue(makeFarm, 0, 0);
    auto result = bq.applyProduction(5);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(bq.accumulatedProduction(), 5);
    EXPECT_FALSE(bq.isEmpty());
}

TEST(BuildQueueTest, ApplyProductionCompletesBuilding) {
    BuildQueue bq;
    // Farm costs 20 production
    bq.enqueue(makeFarm, 3, 4);
    // Apply in two steps
    auto partial = bq.applyProduction(15);
    EXPECT_FALSE(partial.has_value());
    auto completed = bq.applyProduction(5);
    ASSERT_TRUE(completed.has_value());
    EXPECT_EQ(completed->name(), "Farm");
    EXPECT_TRUE(bq.isEmpty());
    EXPECT_EQ(bq.accumulatedProduction(), 0);
}

TEST(BuildQueueTest, ApplyProductionOnEmptyQueueReturnsNullopt) {
    BuildQueue bq;
    auto result = bq.applyProduction(100);
    EXPECT_FALSE(result.has_value());
}

TEST(BuildQueueTest, CancelRemovesFrontItem) {
    BuildQueue bq;
    bq.enqueue(makeMine, 0, 0);
    bq.enqueue(makeMarket, 1, 1);
    bq.applyProduction(5);
    bq.cancel();
    ASSERT_TRUE(bq.currentItem().has_value());
    EXPECT_EQ(bq.currentItem()->name, "Market");
    EXPECT_EQ(bq.accumulatedProduction(), 0);
}

TEST(BuildQueueTest, CancelOnEmptyQueueIsNoOp) {
    BuildQueue bq;
    bq.cancel(); // should not crash
    EXPECT_TRUE(bq.isEmpty());
}

TEST(BuildQueueTest, TurnsRemainingFarm) {
    BuildQueue bq;
    // Farm costs 20, productionPerTurn=5 => ceil((20-0)/5)=4 turns
    bq.enqueue(makeFarm, 0, 0);
    EXPECT_EQ(bq.turnsRemaining(5), 4);
}

TEST(BuildQueueTest, TurnsRemainingWithAccumulated) {
    BuildQueue bq;
    // Mine costs 10, after 3 accumulated => ceil((10-3)/5)=ceil(7/5)=2 turns
    bq.enqueue(makeMine, 0, 0);
    bq.applyProduction(3);
    EXPECT_EQ(bq.turnsRemaining(5), 2);
}

TEST(BuildQueueTest, TurnsRemainingOnEmptyQueueIsZero) {
    BuildQueue bq;
    EXPECT_EQ(bq.turnsRemaining(5), 0);
}

TEST(BuildQueueTest, TurnsRemainingWithZeroProductionIsZero) {
    BuildQueue bq;
    bq.enqueue(makeMarket, 0, 0);
    EXPECT_EQ(bq.turnsRemaining(0), 0);
}

TEST(BuildQueueTest, MineCompletesAtCorrectCost) {
    BuildQueue bq;
    // Mine costs 10 production
    bq.enqueue(makeMine, 2, 3);
    auto partial = bq.applyProduction(9);
    EXPECT_FALSE(partial.has_value());
    auto completed = bq.applyProduction(1);
    ASSERT_TRUE(completed.has_value());
    EXPECT_EQ(completed->name(), "Mine");
    EXPECT_TRUE(bq.isEmpty());
}

TEST(BuildQueueTest, MarketCompletesAtCorrectCost) {
    BuildQueue bq;
    // Market costs 30 production
    bq.enqueue(makeMarket, 5, 6);
    auto partial = bq.applyProduction(29);
    EXPECT_FALSE(partial.has_value());
    auto completed = bq.applyProduction(1);
    ASSERT_TRUE(completed.has_value());
    EXPECT_EQ(completed->name(), "Market");
    EXPECT_TRUE(bq.isEmpty());
}

TEST(BuildQueueTest, AccumulatedResetAfterCompletion) {
    BuildQueue bq;
    bq.enqueue(makeMine, 0, 0);
    bq.applyProduction(10); // completes mine
    EXPECT_EQ(bq.accumulatedProduction(), 0);
}

TEST(BuildQueueTest, MultipleItemsInQueue) {
    BuildQueue bq;
    bq.enqueue(makeFarm, 0, 0);
    bq.enqueue(makeMine, 1, 1);
    // Complete the farm (costs 20)
    bq.applyProduction(20);
    ASSERT_FALSE(bq.isEmpty());
    ASSERT_TRUE(bq.currentItem().has_value());
    EXPECT_EQ(bq.currentItem()->name, "Mine");
}
