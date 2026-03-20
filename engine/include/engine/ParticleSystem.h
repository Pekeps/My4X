#pragma once

#include "raylib.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine {

// ── Particle type enum ──────────────────────────────────────────────────────

/// Types of particle effects available in the game.
enum class ParticleType : std::uint8_t {
    CombatHit,
    UnitSpawn,
    BuildingComplete,
    Capture,
    LevelUp,
};

// ── Particle type count ─────────────────────────────────────────────────────

/// Total number of particle types (keep in sync with enum).
static constexpr std::size_t PARTICLE_TYPE_COUNT = 5;

// ── Pool capacity ───────────────────────────────────────────────────────────

/// Maximum number of particles that can be alive at once.
static constexpr std::size_t MAX_PARTICLES = 512;

// ── Particle type properties ────────────────────────────────────────────────

/// Configurable visual properties for each particle type.
struct ParticleTypeProperties {
    /// Base color of the particle.
    Color color{};

    /// Base size (radius) of the particle sphere.
    float size = 0.0F;

    /// Maximum lifetime in seconds.
    float lifetime = 0.0F;

    /// Minimum initial velocity (units per second, each axis).
    float velocityMin = 0.0F;

    /// Maximum initial velocity (units per second, each axis).
    float velocityMax = 0.0F;

    /// Gravity acceleration applied each frame (negative = downward).
    float gravity = 0.0F;

    /// Alpha fade rate (alpha units lost per second; 255 = fully opaque).
    float fadeRate = 0.0F;
};

// ── Default properties per particle type ────────────────────────────────────

// CombatHit – fast red/orange sparks
static constexpr float COMBAT_HIT_SIZE = 0.08F;
static constexpr float COMBAT_HIT_LIFETIME = 0.6F;
static constexpr float COMBAT_HIT_VEL_MIN = -2.0F;
static constexpr float COMBAT_HIT_VEL_MAX = 2.0F;
static constexpr float COMBAT_HIT_GRAVITY = -3.0F;
static constexpr float COMBAT_HIT_FADE_RATE = 400.0F;
static constexpr unsigned char COMBAT_HIT_RED = 255;
static constexpr unsigned char COMBAT_HIT_GREEN = 80;
static constexpr unsigned char COMBAT_HIT_BLUE = 30;
static constexpr unsigned char COMBAT_HIT_ALPHA = 255;

// UnitSpawn – upward blue/white sparkle
static constexpr float UNIT_SPAWN_SIZE = 0.06F;
static constexpr float UNIT_SPAWN_LIFETIME = 0.8F;
static constexpr float UNIT_SPAWN_VEL_MIN = -0.8F;
static constexpr float UNIT_SPAWN_VEL_MAX = 0.8F;
static constexpr float UNIT_SPAWN_GRAVITY = 0.5F;
static constexpr float UNIT_SPAWN_FADE_RATE = 300.0F;
static constexpr unsigned char UNIT_SPAWN_RED = 100;
static constexpr unsigned char UNIT_SPAWN_GREEN = 180;
static constexpr unsigned char UNIT_SPAWN_BLUE = 255;
static constexpr unsigned char UNIT_SPAWN_ALPHA = 255;

// BuildingComplete – golden burst
static constexpr float BUILDING_COMPLETE_SIZE = 0.07F;
static constexpr float BUILDING_COMPLETE_LIFETIME = 1.0F;
static constexpr float BUILDING_COMPLETE_VEL_MIN = -1.2F;
static constexpr float BUILDING_COMPLETE_VEL_MAX = 1.2F;
static constexpr float BUILDING_COMPLETE_GRAVITY = -1.0F;
static constexpr float BUILDING_COMPLETE_FADE_RATE = 240.0F;
static constexpr unsigned char BUILDING_COMPLETE_RED = 255;
static constexpr unsigned char BUILDING_COMPLETE_GREEN = 215;
static constexpr unsigned char BUILDING_COMPLETE_BLUE = 0;
static constexpr unsigned char BUILDING_COMPLETE_ALPHA = 255;

// Capture – purple swirl
static constexpr float CAPTURE_SIZE = 0.06F;
static constexpr float CAPTURE_LIFETIME = 0.9F;
static constexpr float CAPTURE_VEL_MIN = -1.0F;
static constexpr float CAPTURE_VEL_MAX = 1.0F;
static constexpr float CAPTURE_GRAVITY = 0.2F;
static constexpr float CAPTURE_FADE_RATE = 270.0F;
static constexpr unsigned char CAPTURE_RED = 180;
static constexpr unsigned char CAPTURE_GREEN = 80;
static constexpr unsigned char CAPTURE_BLUE = 255;
static constexpr unsigned char CAPTURE_ALPHA = 255;

// LevelUp – bright green upward stream
static constexpr float LEVEL_UP_SIZE = 0.05F;
static constexpr float LEVEL_UP_LIFETIME = 1.2F;
static constexpr float LEVEL_UP_VEL_MIN = -0.6F;
static constexpr float LEVEL_UP_VEL_MAX = 0.6F;
static constexpr float LEVEL_UP_GRAVITY = 1.0F;
static constexpr float LEVEL_UP_FADE_RATE = 200.0F;
static constexpr unsigned char LEVEL_UP_RED = 50;
static constexpr unsigned char LEVEL_UP_GREEN = 255;
static constexpr unsigned char LEVEL_UP_BLUE = 100;
static constexpr unsigned char LEVEL_UP_ALPHA = 255;

/// Returns default properties for a given particle type.
[[nodiscard]] ParticleTypeProperties defaultPropertiesFor(ParticleType type);

// ── Single particle ─────────────────────────────────────────────────────────

/// A single particle within the object pool. Active particles have
/// `alive == true`; dead particles are reused on the next emit.
struct Particle {
    float posX = 0.0F;
    float posY = 0.0F;
    float posZ = 0.0F;

    float velX = 0.0F;
    float velY = 0.0F;
    float velZ = 0.0F;

    float size = 0.0F;
    float lifetime = 0.0F;
    float age = 0.0F;
    float gravity = 0.0F;
    float fadeRate = 0.0F;

    Color color{};
    bool alive = false;
};

// ── Particle system ─────────────────────────────────────────────────────────

/// A lightweight particle system backed by a fixed-size object pool.
/// Pre-allocates all particles at construction, reuses dead ones on emit.
/// No runtime heap allocation during normal operation.
class ParticleSystem {
  public:
    /// Emit `count` particles of the given type at a world position.
    /// Uses the default properties for the particle type.
    void emit(float worldX, float worldY, float worldZ, ParticleType type, std::size_t count);

    /// Emit `count` particles with explicit properties at a world position.
    void emit(float worldX, float worldY, float worldZ, const ParticleTypeProperties &props, std::size_t count);

    /// Update all active particles by `deltaTime` seconds.
    void update(float deltaTime);

    /// Draw all active particles as 3D spheres. Must be called inside BeginMode3D.
    void draw() const;

    /// Return the number of currently active (alive) particles.
    [[nodiscard]] std::size_t activeCount() const;

    /// Return the pool capacity.
    [[nodiscard]] static constexpr std::size_t capacity() { return MAX_PARTICLES; }

  private:
    /// Find the next dead particle slot. Returns MAX_PARTICLES if pool is full.
    [[nodiscard]] std::size_t findDeadSlot() const;

    /// The pre-allocated particle pool.
    std::array<Particle, MAX_PARTICLES> pool_{};

    /// Hint index for the next free slot search start.
    mutable std::size_t searchHint_ = 0;
};

} // namespace engine
