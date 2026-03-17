#include "game/Resource.h"

#include <gtest/gtest.h>

TEST(ResourceTest, DefaultIsZero) {
    game::Resource res;
    EXPECT_EQ(res.gold, 0);
    EXPECT_EQ(res.production, 0);
    EXPECT_EQ(res.food, 0);
}

TEST(ResourceTest, AggregateInit) {
    game::Resource res{10, 20, 30};
    EXPECT_EQ(res.gold, 10);
    EXPECT_EQ(res.production, 20);
    EXPECT_EQ(res.food, 30);
}

TEST(ResourceTest, Addition) {
    game::Resource a{1, 2, 3};
    game::Resource b{4, 5, 6};
    game::Resource result = a + b;
    EXPECT_EQ(result, (game::Resource{5, 7, 9}));
}

TEST(ResourceTest, Subtraction) {
    game::Resource a{10, 20, 30};
    game::Resource b{3, 5, 7};
    game::Resource result = a - b;
    EXPECT_EQ(result, (game::Resource{7, 15, 23}));
}

TEST(ResourceTest, SubtractionNegativeResult) {
    game::Resource a{1, 2, 3};
    game::Resource b{5, 5, 5};
    game::Resource result = a - b;
    EXPECT_EQ(result, (game::Resource{-4, -3, -2}));
}

TEST(ResourceTest, PlusEquals) {
    game::Resource a{1, 2, 3};
    a += game::Resource{10, 20, 30};
    EXPECT_EQ(a, (game::Resource{11, 22, 33}));
}

TEST(ResourceTest, MinusEquals) {
    game::Resource a{10, 20, 30};
    a -= game::Resource{3, 5, 7};
    EXPECT_EQ(a, (game::Resource{7, 15, 23}));
}

TEST(ResourceTest, GreaterEqualAllFieldsGreater) {
    game::Resource a{10, 20, 30};
    game::Resource b{5, 10, 15};
    EXPECT_TRUE(a >= b);
}

TEST(ResourceTest, GreaterEqualExactlyEqual) {
    game::Resource a{5, 10, 15};
    game::Resource b{5, 10, 15};
    EXPECT_TRUE(a >= b);
}

TEST(ResourceTest, GreaterEqualOneFieldLess) {
    game::Resource a{10, 5, 30};
    game::Resource b{10, 10, 30};
    EXPECT_FALSE(a >= b);
}

TEST(ResourceTest, GreaterEqualAllFieldsLess) {
    game::Resource a{1, 1, 1};
    game::Resource b{5, 5, 5};
    EXPECT_FALSE(a >= b);
}

TEST(ResourceTest, GreaterEqualWithZero) {
    game::Resource a{0, 0, 0};
    game::Resource b{0, 0, 0};
    EXPECT_TRUE(a >= b);
}

TEST(ResourceTest, GreaterEqualWithNegative) {
    game::Resource a{-1, 5, 5};
    game::Resource b{0, 0, 0};
    EXPECT_FALSE(a >= b);
}

TEST(ResourceTest, Equality) {
    game::Resource a{1, 2, 3};
    game::Resource b{1, 2, 3};
    EXPECT_EQ(a, b);
}

TEST(ResourceTest, InequalityDifferentGold) {
    game::Resource a{1, 2, 3};
    game::Resource b{99, 2, 3};
    EXPECT_NE(a, b);
}

TEST(ResourceTest, AdditionWithZero) {
    game::Resource a{5, 10, 15};
    game::Resource zero;
    EXPECT_EQ(a + zero, a);
    EXPECT_EQ(zero + a, a);
}

TEST(ResourceTest, AdditionWithNegative) {
    game::Resource a{5, 10, 15};
    game::Resource b{-5, -10, -15};
    EXPECT_EQ(a + b, (game::Resource{0, 0, 0}));
}
