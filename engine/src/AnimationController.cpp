#include "engine/AnimationController.h"

#include <algorithm>
#include <utility>

namespace engine {

// ── State transition table ──────────────────────────────────────────────────

bool AnimationController::isValidTransition(AnimationState from, AnimationState to) {
    // Dead is a terminal state — no transitions out except to Dead (no-op).
    if (from == AnimationState::Dead) {
        return to == AnimationState::Dead;
    }

    // Dying can only transition to Dead (auto-transition on completion).
    if (from == AnimationState::Dying) {
        return to == AnimationState::Dead;
    }

    // Any living state can transition to Dying (death can happen any time).
    if (to == AnimationState::Dying) {
        return true;
    }

    // Cannot transition directly to Dead — must go through Dying first.
    if (to == AnimationState::Dead) {
        return false;
    }

    // All other transitions among living states are allowed.
    return true;
}

// ── Duration lookup ─────────────────────────────────────────────────────────

float AnimationController::defaultDuration(AnimationState state) {
    switch (state) {
    case AnimationState::Idle:
        return IDLE_ANIMATION_DURATION;
    case AnimationState::Walking:
        return WALKING_ANIMATION_DURATION;
    case AnimationState::Attacking:
        return ATTACKING_ANIMATION_DURATION;
    case AnimationState::TakingDamage:
        return TAKING_DAMAGE_ANIMATION_DURATION;
    case AnimationState::Dying:
        return DYING_ANIMATION_DURATION;
    case AnimationState::Dead:
        return DEAD_ANIMATION_DURATION;
    }
    return IDLE_ANIMATION_DURATION; // fallback
}

// ── Play animation ──────────────────────────────────────────────────────────

bool AnimationController::playAnimation(EntityId entityId, AnimationState state) {
    auto iter = entries_.find(entityId);

    if (iter == entries_.end()) {
        // New entity — always accept.
        entries_[entityId] = AnimationEntry{
            .state = state,
            .elapsed = INITIAL_PROGRESS,
            .duration = defaultDuration(state),
        };
        return true;
    }

    if (!isValidTransition(iter->second.state, state)) {
        return false;
    }

    // Same-state re-trigger resets progress.
    iter->second.state = state;
    iter->second.elapsed = INITIAL_PROGRESS;
    iter->second.duration = defaultDuration(state);
    return true;
}

// ── Update ──────────────────────────────────────────────────────────────────

void AnimationController::update(float deltaTime) {
    for (auto &[entityId, entry] : entries_) {
        // Dead state does not advance.
        if (entry.state == AnimationState::Dead) {
            continue;
        }

        entry.elapsed += deltaTime;

        // Check for completion of one-shot animations.
        if (entry.duration > INITIAL_PROGRESS && entry.elapsed >= entry.duration) {
            handleCompletion(entityId, entry);
        }
    }
}

// ── Query methods ───────────────────────────────────────────────────────────

std::optional<AnimationState> AnimationController::getState(EntityId entityId) const {
    auto iter = entries_.find(entityId);
    if (iter == entries_.end()) {
        return std::nullopt;
    }
    return iter->second.state;
}

std::optional<float> AnimationController::getProgress(EntityId entityId) const {
    auto iter = entries_.find(entityId);
    if (iter == entries_.end()) {
        return std::nullopt;
    }

    const auto &entry = iter->second;

    // Dead state has no meaningful progress.
    if (entry.state == AnimationState::Dead) {
        return MAX_PROGRESS;
    }

    // Avoid division by zero for zero-duration states.
    if (entry.duration <= INITIAL_PROGRESS) {
        return MAX_PROGRESS;
    }

    float progress = entry.elapsed / entry.duration;

    // Clamp to [0, 1] for looping states where elapsed may exceed duration
    // briefly before the next update handles wrap-around.
    progress = std::min(progress, MAX_PROGRESS);

    return progress;
}

void AnimationController::removeEntity(EntityId entityId) { entries_.erase(entityId); }

std::size_t AnimationController::entityCount() const { return entries_.size(); }

void AnimationController::setCompletionCallback(CompletionCallback callback) {
    completionCallback_ = std::move(callback);
}

// ── Completion handling ─────────────────────────────────────────────────────

void AnimationController::handleCompletion(EntityId entityId, AnimationEntry &entry) {
    AnimationState finishedState = entry.state;

    switch (entry.state) {
    case AnimationState::Attacking:
    case AnimationState::TakingDamage:
        // One-shot → return to Idle.
        entry.state = AnimationState::Idle;
        entry.elapsed = INITIAL_PROGRESS;
        entry.duration = defaultDuration(AnimationState::Idle);
        break;

    case AnimationState::Dying:
        // Dying → Dead (terminal).
        entry.state = AnimationState::Dead;
        entry.elapsed = INITIAL_PROGRESS;
        entry.duration = DEAD_ANIMATION_DURATION;
        break;

    case AnimationState::Idle:
    case AnimationState::Walking:
        // Looping states — wrap elapsed time.
        while (entry.elapsed >= entry.duration) {
            entry.elapsed -= entry.duration;
        }
        break;

    case AnimationState::Dead:
        // Should not reach here, but guard against it.
        break;
    }

    if (completionCallback_) {
        completionCallback_(entityId, finishedState);
    }
}

} // namespace engine
