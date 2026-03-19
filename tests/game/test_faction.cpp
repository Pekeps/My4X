// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "game/Faction.h"

#include <gtest/gtest.h>

TEST(FactionTest, ConstructionAndAccessors) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    EXPECT_EQ(faction.id(), 1);
    EXPECT_EQ(faction.name(), "Rome");
    EXPECT_EQ(faction.type(), game::FactionType::Player);
    EXPECT_EQ(faction.colorIndex(), 0);
}

TEST(FactionTest, StockpileDefaultsToZero) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    EXPECT_EQ(faction.stockpile(), (game::Resource{0, 0, 0}));
}

TEST(FactionTest, AddResources) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    faction.addResources({10, 20, 30});
    EXPECT_EQ(faction.stockpile(), (game::Resource{10, 20, 30}));
    faction.addResources({5, 5, 5});
    EXPECT_EQ(faction.stockpile(), (game::Resource{15, 25, 35}));
}

TEST(FactionTest, SpendResourcesSuccess) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    faction.addResources({100, 100, 100});
    EXPECT_TRUE(faction.spendResources({30, 40, 50}));
    EXPECT_EQ(faction.stockpile(), (game::Resource{70, 60, 50}));
}

TEST(FactionTest, SpendResourcesInsufficientFunds) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    faction.addResources({10, 10, 10});
    EXPECT_FALSE(faction.spendResources({20, 5, 5}));
    // Stockpile should be unchanged after failed spend
    EXPECT_EQ(faction.stockpile(), (game::Resource{10, 10, 10}));
}

TEST(FactionTest, SpendExactAmount) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    faction.addResources({50, 50, 50});
    EXPECT_TRUE(faction.spendResources({50, 50, 50}));
    EXPECT_EQ(faction.stockpile(), (game::Resource{0, 0, 0}));
}

TEST(FactionTest, FactionTypeNeutralHostile) {
    game::Faction faction(2, "Barbarians", game::FactionType::NeutralHostile, 3);
    EXPECT_EQ(faction.type(), game::FactionType::NeutralHostile);
    EXPECT_EQ(faction.name(), "Barbarians");
    EXPECT_EQ(faction.colorIndex(), 3);
}

TEST(FactionTest, FactionTypeNeutralPassive) {
    game::Faction faction(3, "City-State", game::FactionType::NeutralPassive, 5);
    EXPECT_EQ(faction.type(), game::FactionType::NeutralPassive);
}

TEST(FactionTest, SetName) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    faction.setName("Roman Empire");
    EXPECT_EQ(faction.name(), "Roman Empire");
}

TEST(FactionTest, SetColorIndex) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    faction.setColorIndex(7);
    EXPECT_EQ(faction.colorIndex(), 7);
}

TEST(FactionTest, MutableStockpile) {
    game::Faction faction(1, "Rome", game::FactionType::Player, 0);
    faction.stockpile() = game::Resource{42, 13, 7};
    EXPECT_EQ(faction.stockpile(), (game::Resource{42, 13, 7}));
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
