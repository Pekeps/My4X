#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace engine {

// ── Animation type enumeration ──────────────────────────────────────────────

/// The kind of unit animation currently playing.
enum class UnitAnimationType : std::uint8_t {
    None,
    Move,
    Attack,
    Death,
};

// ── 3D vector type (engine-internal, avoids raylib dependency in header) ────

/// Lightweight 3D position used by the animator.
struct Vec3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

// ── Animation duration constants ────────────────────────────────────────────

/// Duration of the movement interpolation (seconds).
static constexpr float MOVE_ANIM_DURATION = 0.4F;

/// Duration of the attack lunge animation (seconds).
static constexpr float ATTACK_ANIM_DURATION = 0.35F;

/// Duration of the death shrink/fade animation (seconds).
static constexpr float DEATH_ANIM_DURATION = 0.5F;

/// Maximum forward lunge distance for the attack animation (world units).
static constexpr float ATTACK_LUNGE_DISTANCE = 0.4F;

/// Progress value at which the attack lunge reaches maximum forward offset.
/// The lunge goes forward during [0, LUNGE_PEAK] and returns during
/// [LUNGE_PEAK, 1].
static constexpr float ATTACK_LUNGE_PEAK = 0.35F;

/// Initial progress for all animations.
static constexpr float ANIM_INITIAL_PROGRESS = 0.0F;

/// Completed progress for all animations.
static constexpr float ANIM_MAX_PROGRESS = 1.0F;

/// Default visual scale (no scaling).
static constexpr float DEFAULT_VISUAL_SCALE = 1.0F;

/// Minimum visual scale at the end of the death animation.
static constexpr float DEATH_END_SCALE = 0.0F;

/// Y-axis sink distance during death animation (world units).
static constexpr float DEATH_SINK_DISTANCE = 0.5F;

// ── Entity ID type ──────────────────────────────────────────────────────────

/// Opaque unit identifier used to track animations.
using UnitId = std::uint32_t;

// ── Per-unit animation data ─────────────────────────────────────────────────

/// Internal state for one active animation.
struct UnitAnimationEntry {
    UnitAnimationType type = UnitAnimationType::None;
    float elapsed = 0.0F;
    float duration = 0.0F;

    /// Source / destination positions (Move), or base / target (Attack), or
    /// position at time of death (Death).
    Vec3 from{};
    Vec3 to{};

    /// Cached current visual position (updated each frame by update()).
    Vec3 visualPos{};

    /// Cached current visual scale (updated each frame by update()).
    float visualScale = DEFAULT_VISUAL_SCALE;

    /// Whether this animation has finished.
    bool finished = false;
};

// ── UnitAnimator ────────────────────────────────────────────────────────────

/// Manages smooth visual animations for unit actions (move, attack, death).
/// The animator is purely visual — game-logic positions are updated
/// immediately; the animator only tracks where a unit should be *drawn*.
class UnitAnimator {
  public:
    /// Begin a smooth move interpolation from `fromPos` to `toPos`.
    void startMove(UnitId unitId, Vec3 fromPos, Vec3 toPos);

    /// Begin an attack lunge from the unit's position toward `targetPos`.
    void startAttack(UnitId unitId, Vec3 unitPos, Vec3 targetPos);

    /// Begin a death shrink/sink animation at the given position.
    void startDeath(UnitId unitId, Vec3 pos);

    /// Advance all active animations by `deltaTime` seconds.
    void update(float deltaTime);

    /// Return the animated world position for the given unit. If the unit
    /// has no active animation, returns std::nullopt (caller should fall
    /// back to the logical tile position).
    [[nodiscard]] std::optional<Vec3> getVisualPosition(UnitId unitId) const;

    /// Return the visual scale factor for the given unit (1.0 normally,
    /// shrinks toward 0 during death). Returns std::nullopt if no animation.
    [[nodiscard]] std::optional<float> getVisualScale(UnitId unitId) const;

    /// Return true if the given unit has an active (non-finished) animation.
    [[nodiscard]] bool isAnimating(UnitId unitId) const;

    /// Return the number of active animation entries (for testing).
    [[nodiscard]] std::size_t activeCount() const;

    /// Remove finished animations from the internal map (housekeeping).
    void removeFinished();

  private:
    std::unordered_map<UnitId, UnitAnimationEntry> entries_;

    /// Compute interpolated values for a move animation.
    static void updateMove(UnitAnimationEntry &entry);

    /// Compute interpolated values for an attack lunge animation.
    static void updateAttack(UnitAnimationEntry &entry);

    /// Compute interpolated values for a death animation.
    static void updateDeath(UnitAnimationEntry &entry);
};

} // namespace engine
