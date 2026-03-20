// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/ParticleSystem.h"

#include <gtest/gtest.h>

// ── Construction ─────────────────────────────────────────────────────────────

TEST(ParticleSystemTest, InitiallyEmpty) {
    engine::ParticleSystem ps;
    EXPECT_EQ(ps.activeCount(), 0);
}

TEST(ParticleSystemTest, CapacityMatchesMaxParticles) {
    EXPECT_EQ(engine::ParticleSystem::capacity(), engine::MAX_PARTICLES);
}

// ── Emit by type ─────────────────────────────────────────────────────────────

TEST(ParticleSystemTest, EmitByType_IncreasesActiveCount) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::CombatHit, 10);
    EXPECT_EQ(ps.activeCount(), 10);
}

TEST(ParticleSystemTest, EmitMultipleTypes_AccumulatesCount) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::CombatHit, 5);
    ps.emit(1.0F, 0.0F, 0.0F, engine::ParticleType::UnitSpawn, 3);
    ps.emit(2.0F, 0.0F, 0.0F, engine::ParticleType::LevelUp, 2);
    EXPECT_EQ(ps.activeCount(), 10);
}

// ── Emit by properties ───────────────────────────────────────────────────────

TEST(ParticleSystemTest, EmitByProperties_IncreasesActiveCount) {
    engine::ParticleSystem ps;
    engine::ParticleTypeProperties props{
        .color = {255, 0, 0, 255},
        .size = 0.1F,
        .lifetime = 1.0F,
        .velocityMin = -1.0F,
        .velocityMax = 1.0F,
        .gravity = 0.0F,
        .fadeRate = 200.0F,
    };
    ps.emit(0.0F, 0.0F, 0.0F, props, 7);
    EXPECT_EQ(ps.activeCount(), 7);
}

// ── Emit zero count ──────────────────────────────────────────────────────────

TEST(ParticleSystemTest, EmitZero_DoesNothing) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::CombatHit, 0);
    EXPECT_EQ(ps.activeCount(), 0);
}

// ── Pool limit ───────────────────────────────────────────────────────────────

TEST(ParticleSystemTest, EmitBeyondCapacity_ClampsToMax) {
    engine::ParticleSystem ps;
    // Try to emit more than capacity.
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::UnitSpawn, engine::MAX_PARTICLES + 100);
    EXPECT_EQ(ps.activeCount(), engine::MAX_PARTICLES);
}

// ── Update: particles survive small delta ────────────────────────────────────

TEST(ParticleSystemTest, UpdateSmallDelta_ParticlesSurvive) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::BuildingComplete, 5);
    ps.update(0.01F);
    EXPECT_EQ(ps.activeCount(), 5);
}

// ── Update: particles expire after lifetime ──────────────────────────────────

TEST(ParticleSystemTest, ParticlesExpire_AfterLifetime) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::CombatHit, 3);

    // CombatHit lifetime is 0.6s. Update well past that.
    ps.update(engine::COMBAT_HIT_LIFETIME + 0.1F);
    EXPECT_EQ(ps.activeCount(), 0);
}

TEST(ParticleSystemTest, UnitSpawnParticlesExpire_AfterLifetime) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::UnitSpawn, 4);

    ps.update(engine::UNIT_SPAWN_LIFETIME + 0.1F);
    EXPECT_EQ(ps.activeCount(), 0);
}

TEST(ParticleSystemTest, BuildingCompleteParticlesExpire_AfterLifetime) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::BuildingComplete, 2);

    ps.update(engine::BUILDING_COMPLETE_LIFETIME + 0.1F);
    EXPECT_EQ(ps.activeCount(), 0);
}

TEST(ParticleSystemTest, CaptureParticlesExpire_AfterLifetime) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::Capture, 6);

    ps.update(engine::CAPTURE_LIFETIME + 0.1F);
    EXPECT_EQ(ps.activeCount(), 0);
}

TEST(ParticleSystemTest, LevelUpParticlesExpire_AfterLifetime) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::LevelUp, 3);

    ps.update(engine::LEVEL_UP_LIFETIME + 0.1F);
    EXPECT_EQ(ps.activeCount(), 0);
}

// ── Frame-rate independence ──────────────────────────────────────────────────

TEST(ParticleSystemTest, FrameRateIndependence_ManySmallUpdatesExpireParticles) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::CombatHit, 5);

    // Simulate ~60 FPS for longer than particle lifetime.
    float totalTime = 0.0F;
    while (totalTime < engine::COMBAT_HIT_LIFETIME + 0.1F) {
        ps.update(0.016F);
        totalTime += 0.016F;
    }
    EXPECT_EQ(ps.activeCount(), 0);
}

// ── Particles survive partial lifetime ───────────────────────────────────────

TEST(ParticleSystemTest, ParticlesSurvive_BeforeLifetimeExpires) {
    engine::ParticleSystem ps;
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::LevelUp, 4);

    // LevelUp lifetime is 1.2s. Update to half that.
    ps.update(engine::LEVEL_UP_LIFETIME * 0.5F);
    EXPECT_EQ(ps.activeCount(), 4);
}

// ── Dead slot reuse (object pool) ────────────────────────────────────────────

TEST(ParticleSystemTest, DeadSlotsAreReused) {
    engine::ParticleSystem ps;

    // Fill pool to capacity.
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::CombatHit, engine::MAX_PARTICLES);
    EXPECT_EQ(ps.activeCount(), engine::MAX_PARTICLES);

    // Expire all particles.
    ps.update(engine::COMBAT_HIT_LIFETIME + 0.1F);
    EXPECT_EQ(ps.activeCount(), 0);

    // Emit again — should reuse dead slots without issue.
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::UnitSpawn, 10);
    EXPECT_EQ(ps.activeCount(), 10);
}

// ── Mixed emission and expiration ────────────────────────────────────────────

TEST(ParticleSystemTest, MixedEmitAndExpire_IndependentLifetimes) {
    engine::ParticleSystem ps;

    // Emit CombatHit (0.6s lifetime) and LevelUp (1.2s lifetime).
    ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::CombatHit, 3);
    ps.emit(1.0F, 0.0F, 0.0F, engine::ParticleType::LevelUp, 2);
    EXPECT_EQ(ps.activeCount(), 5);

    // Advance past CombatHit lifetime but not LevelUp.
    ps.update(engine::COMBAT_HIT_LIFETIME + 0.05F);
    EXPECT_EQ(ps.activeCount(), 2); // Only LevelUp particles remain.

    // Advance past LevelUp lifetime.
    float remaining = engine::LEVEL_UP_LIFETIME - engine::COMBAT_HIT_LIFETIME;
    ps.update(remaining + 0.1F);
    EXPECT_EQ(ps.activeCount(), 0);
}

// ── Default properties lookup ────────────────────────────────────────────────

TEST(ParticleSystemTest, DefaultProperties_CombatHit) {
    auto props = engine::defaultPropertiesFor(engine::ParticleType::CombatHit);
    EXPECT_FLOAT_EQ(props.size, engine::COMBAT_HIT_SIZE);
    EXPECT_FLOAT_EQ(props.lifetime, engine::COMBAT_HIT_LIFETIME);
    EXPECT_FLOAT_EQ(props.gravity, engine::COMBAT_HIT_GRAVITY);
}

TEST(ParticleSystemTest, DefaultProperties_UnitSpawn) {
    auto props = engine::defaultPropertiesFor(engine::ParticleType::UnitSpawn);
    EXPECT_FLOAT_EQ(props.size, engine::UNIT_SPAWN_SIZE);
    EXPECT_FLOAT_EQ(props.lifetime, engine::UNIT_SPAWN_LIFETIME);
}

TEST(ParticleSystemTest, DefaultProperties_BuildingComplete) {
    auto props = engine::defaultPropertiesFor(engine::ParticleType::BuildingComplete);
    EXPECT_FLOAT_EQ(props.size, engine::BUILDING_COMPLETE_SIZE);
    EXPECT_FLOAT_EQ(props.lifetime, engine::BUILDING_COMPLETE_LIFETIME);
}

TEST(ParticleSystemTest, DefaultProperties_Capture) {
    auto props = engine::defaultPropertiesFor(engine::ParticleType::Capture);
    EXPECT_FLOAT_EQ(props.size, engine::CAPTURE_SIZE);
    EXPECT_FLOAT_EQ(props.lifetime, engine::CAPTURE_LIFETIME);
}

TEST(ParticleSystemTest, DefaultProperties_LevelUp) {
    auto props = engine::defaultPropertiesFor(engine::ParticleType::LevelUp);
    EXPECT_FLOAT_EQ(props.size, engine::LEVEL_UP_SIZE);
    EXPECT_FLOAT_EQ(props.lifetime, engine::LEVEL_UP_LIFETIME);
}

// ── Consecutive emit–expire cycles ───────────────────────────────────────────

TEST(ParticleSystemTest, MultipleCycles_PoolRemainsConsistent) {
    engine::ParticleSystem ps;

    for (int cycle = 0; cycle < 5; ++cycle) {
        ps.emit(0.0F, 0.0F, 0.0F, engine::ParticleType::CombatHit, 50);
        EXPECT_EQ(ps.activeCount(), 50);

        ps.update(engine::COMBAT_HIT_LIFETIME + 0.1F);
        EXPECT_EQ(ps.activeCount(), 0);
    }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
