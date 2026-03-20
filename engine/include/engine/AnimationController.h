#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>

namespace engine {

// ── Animation states ────────────────────────────────────────────────────────

/// All possible animation states for an entity (unit or building).
enum class AnimationState : std::uint8_t {
    Idle,
    Walking,
    Attacking,
    TakingDamage,
    Dying,
    Dead,
};

// ── Default animation durations (seconds) ───────────────────────────────────

/// Duration of the idle animation loop.
static constexpr float IDLE_ANIMATION_DURATION = 1.0F;

/// Duration of the walking animation loop.
static constexpr float WALKING_ANIMATION_DURATION = 0.8F;

/// Duration of the attack animation (one-shot).
static constexpr float ATTACKING_ANIMATION_DURATION = 0.5F;

/// Duration of the taking-damage animation (one-shot).
static constexpr float TAKING_DAMAGE_ANIMATION_DURATION = 0.3F;

/// Duration of the dying animation (one-shot).
static constexpr float DYING_ANIMATION_DURATION = 0.6F;

/// Dead state has no meaningful duration — it persists indefinitely.
static constexpr float DEAD_ANIMATION_DURATION = 0.0F;

/// Default progress value for newly entered states.
static constexpr float INITIAL_PROGRESS = 0.0F;

/// Maximum progress value (animation complete).
static constexpr float MAX_PROGRESS = 1.0F;

// ── Animation entry ─────────────────────────────────────────────────────────

/// Per-entity animation tracking data.
struct AnimationEntry {
    AnimationState state = AnimationState::Idle;
    float elapsed = 0.0F;
    float duration = IDLE_ANIMATION_DURATION;
};

// ── Entity ID type ──────────────────────────────────────────────────────────

/// Opaque entity identifier used to track animations.
using EntityId = std::uint32_t;

// ── Animation controller ────────────────────────────────────────────────────

/// Manages per-entity animation state machines. Handles state transitions,
/// timing, progress tracking, and auto-transitions on animation completion.
///
/// This is purely a state machine — actual visual playback is a separate
/// concern handled by renderers.
class AnimationController {
  public:
    /// Optional callback invoked when an animation completes.
    /// Receives the entity ID and the state that just finished.
    using CompletionCallback = std::function<void(EntityId, AnimationState)>;

    /// Trigger a state transition for the given entity. Returns true if the
    /// transition was accepted, false if it was invalid (e.g., Dead → Walking).
    /// If the entity is not yet tracked, it is registered automatically.
    bool playAnimation(EntityId entityId, AnimationState state);

    /// Advance all active animations by deltaTime seconds. Handles
    /// auto-transitions when one-shot animations complete.
    void update(float deltaTime);

    /// Return the current animation state for the given entity, or
    /// std::nullopt if the entity is not tracked.
    [[nodiscard]] std::optional<AnimationState> getState(EntityId entityId) const;

    /// Return the current animation progress (0.0 – 1.0) for the given
    /// entity, or std::nullopt if the entity is not tracked.
    [[nodiscard]] std::optional<float> getProgress(EntityId entityId) const;

    /// Remove an entity from the animation system entirely.
    void removeEntity(EntityId entityId);

    /// Return the number of entities currently tracked (for testing).
    [[nodiscard]] std::size_t entityCount() const;

    /// Set an optional callback invoked when any animation finishes.
    void setCompletionCallback(CompletionCallback callback);

    /// Check whether a transition from `from` to `to` is valid.
    [[nodiscard]] static bool isValidTransition(AnimationState from, AnimationState to);

    /// Return the default duration for a given animation state.
    [[nodiscard]] static float defaultDuration(AnimationState state);

  private:
    std::unordered_map<EntityId, AnimationEntry> entries_;
    CompletionCallback completionCallback_;

    /// Handle auto-transition after a one-shot animation completes.
    void handleCompletion(EntityId entityId, AnimationEntry &entry);
};

} // namespace engine
