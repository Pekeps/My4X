// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/Transition.h"

#include <gtest/gtest.h>

TEST(TransitionTest, InitiallyInactive) {
    engine::Transition t;
    EXPECT_FALSE(t.isActive());
    EXPECT_FALSE(t.isComplete());
}

TEST(TransitionTest, FadeOutStartsActive) {
    engine::Transition t;
    t.start(engine::TransitionDir::FadeOut);
    EXPECT_TRUE(t.isActive());
    EXPECT_FALSE(t.isComplete());
    // At the very start, alpha should be near 0 for FadeOut.
    EXPECT_NEAR(t.alpha(), 0.0F, 0.01F);
}

TEST(TransitionTest, FadeOutCompletesWithFullAlpha) {
    engine::Transition t;
    t.start(engine::TransitionDir::FadeOut);

    // Advance well past the duration.
    t.update(1.0F);
    EXPECT_FALSE(t.isActive());
    EXPECT_TRUE(t.isComplete());
    EXPECT_NEAR(t.alpha(), 1.0F, 0.01F);
}

TEST(TransitionTest, FadeInStartsWithFullAlpha) {
    engine::Transition t;
    t.start(engine::TransitionDir::FadeIn);
    EXPECT_TRUE(t.isActive());
    // At start of FadeIn, alpha should be 1 (fully black).
    EXPECT_NEAR(t.alpha(), 1.0F, 0.01F);
}

TEST(TransitionTest, FadeInCompletesWithZeroAlpha) {
    engine::Transition t;
    t.start(engine::TransitionDir::FadeIn);
    t.update(1.0F);
    EXPECT_FALSE(t.isActive());
    EXPECT_TRUE(t.isComplete());
    EXPECT_NEAR(t.alpha(), 0.0F, 0.01F);
}

TEST(TransitionTest, SmallUpdateKeepsActive) {
    engine::Transition t;
    t.start(engine::TransitionDir::FadeOut);
    t.update(0.01F);
    EXPECT_TRUE(t.isActive());
    EXPECT_FALSE(t.isComplete());
    // Alpha should be between 0 and 1.
    EXPECT_GT(t.alpha(), 0.0F);
    EXPECT_LT(t.alpha(), 1.0F);
}

TEST(TransitionTest, RestartResetsState) {
    engine::Transition t;
    t.start(engine::TransitionDir::FadeOut);
    t.update(1.0F);
    EXPECT_TRUE(t.isComplete());

    // Restart as FadeIn.
    t.start(engine::TransitionDir::FadeIn);
    EXPECT_TRUE(t.isActive());
    EXPECT_FALSE(t.isComplete());
    EXPECT_NEAR(t.alpha(), 1.0F, 0.01F);
}

TEST(TransitionTest, NoUpdateWhenInactive) {
    engine::Transition t;
    // update on an inactive transition should be a no-op.
    t.update(1.0F);
    EXPECT_FALSE(t.isActive());
    EXPECT_FALSE(t.isComplete());
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
