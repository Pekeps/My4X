#pragma once

#include "raylib.h"

#include <cstddef>
#include <string>
#include <vector>

namespace engine {

// ── Timing and visual constants ──────────────────────────────────────────────

/// Duration in seconds for floating damage numbers to remain visible.
static constexpr float DAMAGE_NUMBER_DURATION = 1.5F;

/// Vertical rise speed for floating damage numbers (world units per second).
static constexpr float DAMAGE_NUMBER_RISE_SPEED = 1.2F;

/// Font size for floating damage numbers drawn as 2D overlay.
static constexpr int DAMAGE_NUMBER_FONT_SIZE = 24;

/// Duration in seconds for the hit flash tint on a damaged unit.
static constexpr float HIT_FLASH_DURATION = 0.3F;

/// Duration in seconds for the death shrink/fade effect.
static constexpr float DEATH_EFFECT_DURATION = 0.6F;

/// Starting scale for the death effect (normal size).
static constexpr float DEATH_EFFECT_START_SCALE = 1.0F;

/// Ending scale for the death effect (fully shrunk).
static constexpr float DEATH_EFFECT_END_SCALE = 0.0F;

/// Radius of the death effect sphere.
static constexpr float DEATH_EFFECT_RADIUS = 0.3F;

/// Y offset for the death effect sphere above the ground.
static constexpr float DEATH_EFFECT_Y_OFFSET = 0.4F;

/// Maximum alpha value for damage number text.
static constexpr float DAMAGE_NUMBER_MAX_ALPHA = 255.0F;

/// Maximum alpha value for death effect.
static constexpr float DEATH_EFFECT_MAX_ALPHA = 200.0F;

/// Horizontal offset for counter-attack damage numbers to avoid overlap.
static constexpr float COUNTER_DAMAGE_X_OFFSET = 0.3F;

// ── Floating damage number ───────────────────────────────────────────────────

/// A floating damage number that rises and fades out over time.
struct FloatingDamageNumber {
    /// World-space position where the number originated.
    float worldX = 0.0F;
    float worldY = 0.0F;
    float worldZ = 0.0F;

    /// The damage value to display.
    int damage = 0;

    /// Remaining lifetime in seconds.
    float timeRemaining = 0.0F;

    /// Color of the text (red for normal damage, orange for counter-attack).
    Color color = RED;
};

// ── Hit flash ────────────────────────────────────────────────────────────────

/// A brief color tint applied to a unit's tile when combat resolves.
struct HitFlash {
    int row = -1;
    int col = -1;
    float timeRemaining = 0.0F;
};

// ── Death effect ─────────────────────────────────────────────────────────────

/// A shrink/fade animation played when a unit dies.
struct DeathEffect {
    float worldX = 0.0F;
    float worldY = DEATH_EFFECT_Y_OFFSET;
    float worldZ = 0.0F;
    float timeRemaining = 0.0F;
    Color color = RED;
};

// ── Effect manager ───────────────────────────────────────────────────────────

/// Manages all active combat visual effects, providing a unified update/draw
/// interface. All effects are frame-rate independent (use delta time).
class CombatEffectManager {
  public:
    /// Spawn a floating damage number at the given world position.
    void spawnDamageNumber(float worldX, float worldY, float worldZ, int damage, Color color);

    /// Spawn a hit flash on the given tile.
    void spawnHitFlash(int row, int col);

    /// Spawn a death effect at the given world position.
    void spawnDeathEffect(float worldX, float worldY, float worldZ, Color color);

    /// Update all active effects by the given delta time (seconds).
    void update(float deltaTime);

    /// Draw floating damage numbers as 2D text projected from 3D positions.
    /// Must be called outside of BeginMode3D (in 2D overlay context).
    void drawDamageNumbers(Camera3D camera) const;

    /// Draw hit flashes in 3D space.
    /// Must be called inside BeginMode3D.
    void drawHitFlashes() const;

    /// Draw death effects in 3D space.
    /// Must be called inside BeginMode3D.
    void drawDeathEffects() const;

    /// Return the number of active damage numbers (for testing).
    [[nodiscard]] std::size_t activeDamageNumberCount() const;

    /// Return the number of active hit flashes (for testing).
    [[nodiscard]] std::size_t activeHitFlashCount() const;

    /// Return the number of active death effects (for testing).
    [[nodiscard]] std::size_t activeDeathEffectCount() const;

  private:
    std::vector<FloatingDamageNumber> damageNumbers_;
    std::vector<HitFlash> hitFlashes_;
    std::vector<DeathEffect> deathEffects_;
};

} // namespace engine
