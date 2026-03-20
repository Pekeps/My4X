#include "engine/ParticleSystem.h"

#include "raylib.h"

#include <algorithm>
#include <cstddef>

namespace engine {

// ── Default properties lookup ────────────────────────────────────────────────

ParticleTypeProperties defaultPropertiesFor(ParticleType type) {
    switch (type) {
    case ParticleType::CombatHit:
        return ParticleTypeProperties{
            .color = Color{COMBAT_HIT_RED, COMBAT_HIT_GREEN, COMBAT_HIT_BLUE, COMBAT_HIT_ALPHA},
            .size = COMBAT_HIT_SIZE,
            .lifetime = COMBAT_HIT_LIFETIME,
            .velocityMin = COMBAT_HIT_VEL_MIN,
            .velocityMax = COMBAT_HIT_VEL_MAX,
            .gravity = COMBAT_HIT_GRAVITY,
            .fadeRate = COMBAT_HIT_FADE_RATE,
        };
    case ParticleType::UnitSpawn:
        return ParticleTypeProperties{
            .color = Color{UNIT_SPAWN_RED, UNIT_SPAWN_GREEN, UNIT_SPAWN_BLUE, UNIT_SPAWN_ALPHA},
            .size = UNIT_SPAWN_SIZE,
            .lifetime = UNIT_SPAWN_LIFETIME,
            .velocityMin = UNIT_SPAWN_VEL_MIN,
            .velocityMax = UNIT_SPAWN_VEL_MAX,
            .gravity = UNIT_SPAWN_GRAVITY,
            .fadeRate = UNIT_SPAWN_FADE_RATE,
        };
    case ParticleType::BuildingComplete:
        return ParticleTypeProperties{
            .color =
                Color{BUILDING_COMPLETE_RED, BUILDING_COMPLETE_GREEN, BUILDING_COMPLETE_BLUE, BUILDING_COMPLETE_ALPHA},
            .size = BUILDING_COMPLETE_SIZE,
            .lifetime = BUILDING_COMPLETE_LIFETIME,
            .velocityMin = BUILDING_COMPLETE_VEL_MIN,
            .velocityMax = BUILDING_COMPLETE_VEL_MAX,
            .gravity = BUILDING_COMPLETE_GRAVITY,
            .fadeRate = BUILDING_COMPLETE_FADE_RATE,
        };
    case ParticleType::Capture:
        return ParticleTypeProperties{
            .color = Color{CAPTURE_RED, CAPTURE_GREEN, CAPTURE_BLUE, CAPTURE_ALPHA},
            .size = CAPTURE_SIZE,
            .lifetime = CAPTURE_LIFETIME,
            .velocityMin = CAPTURE_VEL_MIN,
            .velocityMax = CAPTURE_VEL_MAX,
            .gravity = CAPTURE_GRAVITY,
            .fadeRate = CAPTURE_FADE_RATE,
        };
    case ParticleType::LevelUp:
        return ParticleTypeProperties{
            .color = Color{LEVEL_UP_RED, LEVEL_UP_GREEN, LEVEL_UP_BLUE, LEVEL_UP_ALPHA},
            .size = LEVEL_UP_SIZE,
            .lifetime = LEVEL_UP_LIFETIME,
            .velocityMin = LEVEL_UP_VEL_MIN,
            .velocityMax = LEVEL_UP_VEL_MAX,
            .gravity = LEVEL_UP_GRAVITY,
            .fadeRate = LEVEL_UP_FADE_RATE,
        };
    }
    // Unreachable for valid enum values, but satisfy compiler.
    return ParticleTypeProperties{};
}

// ── Emit (by type) ──────────────────────────────────────────────────────────

void ParticleSystem::emit(float worldX, float worldY, float worldZ, ParticleType type, std::size_t count) {
    emit(worldX, worldY, worldZ, defaultPropertiesFor(type), count);
}

// ── Emit (by properties) ────────────────────────────────────────────────────

/// Deterministic velocity spread factor for distributing particles evenly.
static constexpr float VELOCITY_SPREAD_FACTOR = 0.7F;

/// Offset used to generate pseudo-random but deterministic velocity variation.
static constexpr float VELOCITY_HASH_OFFSET = 0.3F;

/// Prime-based spread constant for velocity hashing.
static constexpr float VELOCITY_HASH_PRIME = 7.0F;

/// Divisor for normalizing the hash index into [0, 1].
static constexpr float VELOCITY_HASH_DIVISOR = 13.0F;

void ParticleSystem::emit(float worldX, float worldY, float worldZ, const ParticleTypeProperties &props,
                          std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t slot = findDeadSlot();
        if (slot >= MAX_PARTICLES) {
            return; // Pool is full.
        }

        Particle &p = pool_.at(slot);
        p.posX = worldX;
        p.posY = worldY;
        p.posZ = worldZ;

        // Generate deterministic but varied velocities based on particle index.
        // This avoids runtime random number generation while still giving visual spread.
        float t = static_cast<float>(i) / static_cast<float>(count > 1 ? count : 1);
        float range = props.velocityMax - props.velocityMin;

        // Simple deterministic spread: each axis gets a different offset.
        float hashX = t;
        float hashY = (t * VELOCITY_SPREAD_FACTOR) + VELOCITY_HASH_OFFSET;
        if (hashY > 1.0F) {
            hashY -= 1.0F;
        }
        float hashZ =
            static_cast<float>(static_cast<int>(t * VELOCITY_HASH_PRIME) % static_cast<int>(VELOCITY_HASH_DIVISOR)) /
            VELOCITY_HASH_DIVISOR;

        p.velX = props.velocityMin + (hashX * range);
        p.velY = props.velocityMin + (hashY * range);
        p.velZ = props.velocityMin + (hashZ * range);

        p.size = props.size;
        p.lifetime = props.lifetime;
        p.age = 0.0F;
        p.gravity = props.gravity;
        p.fadeRate = props.fadeRate;
        p.color = props.color;
        p.alive = true;
    }
}

// ── Update ──────────────────────────────────────────────────────────────────

/// Full alpha value for a freshly spawned particle.
static constexpr float FULL_ALPHA = 255.0F;

/// Minimum alpha below which a particle is considered invisible.
static constexpr float MIN_ALPHA = 0.0F;

void ParticleSystem::update(float deltaTime) {
    for (auto &p : pool_) {
        if (!p.alive) {
            continue;
        }

        p.age += deltaTime;

        // Kill particle if it exceeded its lifetime.
        if (p.age >= p.lifetime) {
            p.alive = false;
            continue;
        }

        // Apply velocity.
        p.posX += p.velX * deltaTime;
        p.posY += p.velY * deltaTime;
        p.posZ += p.velZ * deltaTime;

        // Apply gravity to vertical velocity.
        p.velY += p.gravity * deltaTime;

        // Fade alpha over time.
        float alpha = std::max(FULL_ALPHA - (p.fadeRate * p.age), MIN_ALPHA);
        p.color.a = static_cast<unsigned char>(alpha);
    }
}

// ── Draw ────────────────────────────────────────────────────────────────────

void ParticleSystem::draw() const {
    for (const auto &p : pool_) {
        if (!p.alive) {
            continue;
        }
        Vector3 pos = {p.posX, p.posY, p.posZ};
        DrawSphere(pos, p.size, p.color);
    }
}

// ── Active count ────────────────────────────────────────────────────────────

std::size_t ParticleSystem::activeCount() const {
    std::size_t count = 0;
    for (const auto &p : pool_) {
        if (p.alive) {
            ++count;
        }
    }
    return count;
}

// ── Find dead slot ──────────────────────────────────────────────────────────

std::size_t ParticleSystem::findDeadSlot() const {
    // Start searching from the hint to avoid O(n) scans when emitting bursts.
    for (std::size_t i = 0; i < MAX_PARTICLES; ++i) {
        std::size_t idx = (searchHint_ + i) % MAX_PARTICLES;
        if (!pool_.at(idx).alive) {
            searchHint_ = (idx + 1) % MAX_PARTICLES;
            return idx;
        }
    }
    return MAX_PARTICLES; // Pool is full.
}

} // namespace engine
