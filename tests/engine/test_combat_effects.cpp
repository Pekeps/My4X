// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/CombatEffects.h"

#include <gtest/gtest.h>

// ── Spawn counts ─────────────────────────────────────────────────────────────

TEST(CombatEffectManagerTest, InitiallyEmpty) {
    engine::CombatEffectManager mgr;
    EXPECT_EQ(mgr.activeDamageNumberCount(), 0);
    EXPECT_EQ(mgr.activeHitFlashCount(), 0);
    EXPECT_EQ(mgr.activeDeathEffectCount(), 0);
}

TEST(CombatEffectManagerTest, SpawnDamageNumber_IncreasesCount) {
    engine::CombatEffectManager mgr;
    mgr.spawnDamageNumber(1.0F, 2.0F, 3.0F, 10, RED);
    EXPECT_EQ(mgr.activeDamageNumberCount(), 1);

    mgr.spawnDamageNumber(4.0F, 5.0F, 6.0F, 20, ORANGE);
    EXPECT_EQ(mgr.activeDamageNumberCount(), 2);
}

TEST(CombatEffectManagerTest, SpawnHitFlash_IncreasesCount) {
    engine::CombatEffectManager mgr;
    mgr.spawnHitFlash(3, 5);
    EXPECT_EQ(mgr.activeHitFlashCount(), 1);

    mgr.spawnHitFlash(7, 2);
    EXPECT_EQ(mgr.activeHitFlashCount(), 2);
}

TEST(CombatEffectManagerTest, SpawnDeathEffect_IncreasesCount) {
    engine::CombatEffectManager mgr;
    mgr.spawnDeathEffect(1.0F, 0.4F, 2.0F, RED);
    EXPECT_EQ(mgr.activeDeathEffectCount(), 1);
}

// ── Update and expiration ────────────────────────────────────────────────────

TEST(CombatEffectManagerTest, UpdateSmallDelta_EffectsRemainActive) {
    engine::CombatEffectManager mgr;
    mgr.spawnDamageNumber(0.0F, 1.0F, 0.0F, 15, RED);
    mgr.spawnHitFlash(2, 3);
    mgr.spawnDeathEffect(0.0F, 0.4F, 0.0F, RED);

    // A tiny update should not expire any effects.
    mgr.update(0.01F);
    EXPECT_EQ(mgr.activeDamageNumberCount(), 1);
    EXPECT_EQ(mgr.activeHitFlashCount(), 1);
    EXPECT_EQ(mgr.activeDeathEffectCount(), 1);
}

TEST(CombatEffectManagerTest, HitFlashExpires_AfterDuration) {
    engine::CombatEffectManager mgr;
    mgr.spawnHitFlash(0, 0);

    // Update past the hit flash duration.
    mgr.update(engine::HIT_FLASH_DURATION + 0.01F);
    EXPECT_EQ(mgr.activeHitFlashCount(), 0);
}

TEST(CombatEffectManagerTest, DeathEffectExpires_AfterDuration) {
    engine::CombatEffectManager mgr;
    mgr.spawnDeathEffect(0.0F, 0.4F, 0.0F, RED);

    mgr.update(engine::DEATH_EFFECT_DURATION + 0.01F);
    EXPECT_EQ(mgr.activeDeathEffectCount(), 0);
}

TEST(CombatEffectManagerTest, DamageNumberExpires_AfterDuration) {
    engine::CombatEffectManager mgr;
    mgr.spawnDamageNumber(0.0F, 1.0F, 0.0F, 25, RED);

    mgr.update(engine::DAMAGE_NUMBER_DURATION + 0.01F);
    EXPECT_EQ(mgr.activeDamageNumberCount(), 0);
}

TEST(CombatEffectManagerTest, MultipleEffects_ExpireIndependently) {
    engine::CombatEffectManager mgr;
    mgr.spawnHitFlash(0, 0);                             // short duration
    mgr.spawnDamageNumber(0.0F, 1.0F, 0.0F, 10, RED);   // longer duration
    mgr.spawnDeathEffect(0.0F, 0.4F, 0.0F, RED);        // medium duration

    // Advance past hit flash but not past death effect.
    mgr.update(engine::HIT_FLASH_DURATION + 0.01F);
    EXPECT_EQ(mgr.activeHitFlashCount(), 0);
    EXPECT_EQ(mgr.activeDamageNumberCount(), 1);
    EXPECT_EQ(mgr.activeDeathEffectCount(), 1);

    // Advance past death effect but not past damage number.
    float remaining = engine::DEATH_EFFECT_DURATION - engine::HIT_FLASH_DURATION;
    mgr.update(remaining + 0.01F);
    EXPECT_EQ(mgr.activeDeathEffectCount(), 0);
    EXPECT_EQ(mgr.activeDamageNumberCount(), 1);
}

TEST(CombatEffectManagerTest, FrameRateIndependence_MultipleSmallUpdates) {
    engine::CombatEffectManager mgr;
    mgr.spawnHitFlash(1, 1);

    // Simulate many small frames totaling more than the hit flash duration.
    float totalTime = 0.0F;
    while (totalTime < engine::HIT_FLASH_DURATION + 0.05F) {
        mgr.update(0.016F); // ~60 FPS
        totalTime += 0.016F;
    }
    EXPECT_EQ(mgr.activeHitFlashCount(), 0);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
