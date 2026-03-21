#include "engine/UnitAnimator.h"

#include <algorithm>
#include <cmath>

namespace engine {

// ── Interpolation helpers ───────────────────────────────────────────────────

/// Linear interpolation between two floats.
static float lerp(float a, float b, float t) { return a + ((b - a) * t); }

/// Linear interpolation between two Vec3 positions.
static Vec3 lerpVec3(const Vec3 &a, const Vec3 &b, float t) {
    return {.x = lerp(a.x, b.x, t), .y = lerp(a.y, b.y, t), .z = lerp(a.z, b.z, t)};
}

/// Smooth-step coefficient for the cubic term: 3.0.
static constexpr float SMOOTH_STEP_CUBIC = 3.0F;

/// Smooth-step coefficient for the quadratic term: 2.0.
static constexpr float SMOOTH_STEP_QUADRATIC = 2.0F;

/// Smooth-step easing (ease-in-out) for t in [0, 1].
static float smoothStep(float t) { return t * t * (SMOOTH_STEP_CUBIC - (SMOOTH_STEP_QUADRATIC * t)); }

/// Compute the normalized progress [0, 1] for an entry, clamped.
static float progress(const UnitAnimationEntry &entry) {
    if (entry.duration <= 0.0F) {
        return ANIM_MAX_PROGRESS;
    }
    return std::clamp(entry.elapsed / entry.duration, ANIM_INITIAL_PROGRESS, ANIM_MAX_PROGRESS);
}

// ── startMove ───────────────────────────────────────────────────────────────

void UnitAnimator::startMove(UnitId unitId, Vec3 fromPos, Vec3 toPos) {
    UnitAnimationEntry entry;
    entry.type = UnitAnimationType::Move;
    entry.elapsed = ANIM_INITIAL_PROGRESS;
    entry.duration = MOVE_ANIM_DURATION;
    entry.from = fromPos;
    entry.to = toPos;
    entry.visualPos = fromPos;
    entry.visualScale = DEFAULT_VISUAL_SCALE;
    entry.finished = false;
    entries_[unitId] = entry;
}

// ── startAttack ─────────────────────────────────────────────────────────────

void UnitAnimator::startAttack(UnitId unitId, Vec3 unitPos, Vec3 targetPos) {
    UnitAnimationEntry entry;
    entry.type = UnitAnimationType::Attack;
    entry.elapsed = ANIM_INITIAL_PROGRESS;
    entry.duration = ATTACK_ANIM_DURATION;
    entry.from = unitPos;
    entry.to = targetPos;
    entry.visualPos = unitPos;
    entry.visualScale = DEFAULT_VISUAL_SCALE;
    entry.finished = false;
    entries_[unitId] = entry;
}

// ── startDeath ──────────────────────────────────────────────────────────────

void UnitAnimator::startDeath(UnitId unitId, Vec3 pos) {
    UnitAnimationEntry entry;
    entry.type = UnitAnimationType::Death;
    entry.elapsed = ANIM_INITIAL_PROGRESS;
    entry.duration = DEATH_ANIM_DURATION;
    entry.from = pos;
    entry.to = pos; // death stays in place (sinks)
    entry.visualPos = pos;
    entry.visualScale = DEFAULT_VISUAL_SCALE;
    entry.finished = false;
    entries_[unitId] = entry;
}

// ── Per-type update helpers ─────────────────────────────────────────────────

void UnitAnimator::updateMove(UnitAnimationEntry &entry) {
    float t = smoothStep(progress(entry));
    entry.visualPos = lerpVec3(entry.from, entry.to, t);
    entry.visualScale = DEFAULT_VISUAL_SCALE;
}

void UnitAnimator::updateAttack(UnitAnimationEntry &entry) {
    float t = progress(entry);

    // Compute lunge offset: go forward to peak, then return.
    float lungeFraction = 0.0F;
    if (t <= ATTACK_LUNGE_PEAK) {
        // Forward phase: [0, peak] -> [0, 1]
        lungeFraction = t / ATTACK_LUNGE_PEAK;
    } else {
        // Return phase: [peak, 1] -> [1, 0]
        lungeFraction = (ANIM_MAX_PROGRESS - t) / (ANIM_MAX_PROGRESS - ATTACK_LUNGE_PEAK);
    }

    // Direction from unit to target.
    float dx = entry.to.x - entry.from.x;
    float dz = entry.to.z - entry.from.z;
    float dist = std::sqrt((dx * dx) + (dz * dz));

    Vec3 offset{};
    if (dist > 0.0F) {
        float normX = dx / dist;
        float normZ = dz / dist;
        offset.x = normX * ATTACK_LUNGE_DISTANCE * lungeFraction;
        offset.z = normZ * ATTACK_LUNGE_DISTANCE * lungeFraction;
    }

    entry.visualPos = {.x = entry.from.x + offset.x, .y = entry.from.y + offset.y, .z = entry.from.z + offset.z};
    entry.visualScale = DEFAULT_VISUAL_SCALE;
}

void UnitAnimator::updateDeath(UnitAnimationEntry &entry) {
    float t = progress(entry);

    // Scale shrinks from 1 to 0.
    entry.visualScale = lerp(DEFAULT_VISUAL_SCALE, DEATH_END_SCALE, t);

    // Sink below ground.
    entry.visualPos = entry.from;
    entry.visualPos.y = entry.from.y - (DEATH_SINK_DISTANCE * t);
}

// ── update ──────────────────────────────────────────────────────────────────

void UnitAnimator::update(float deltaTime) {
    for (auto &[id, entry] : entries_) {
        if (entry.finished) {
            continue;
        }

        entry.elapsed += deltaTime;

        switch (entry.type) {
        case UnitAnimationType::Move:
            updateMove(entry);
            break;
        case UnitAnimationType::Attack:
            updateAttack(entry);
            break;
        case UnitAnimationType::Death:
            updateDeath(entry);
            break;
        case UnitAnimationType::None:
            break;
        }

        if (entry.elapsed >= entry.duration) {
            entry.finished = true;
        }
    }
}

// ── Queries ─────────────────────────────────────────────────────────────────

std::optional<Vec3> UnitAnimator::getVisualPosition(UnitId unitId) const {
    auto it = entries_.find(unitId);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    return it->second.visualPos;
}

std::optional<float> UnitAnimator::getVisualScale(UnitId unitId) const {
    auto it = entries_.find(unitId);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    return it->second.visualScale;
}

bool UnitAnimator::isAnimating(UnitId unitId) const {
    auto it = entries_.find(unitId);
    if (it == entries_.end()) {
        return false;
    }
    return !it->second.finished;
}

std::size_t UnitAnimator::activeCount() const {
    std::size_t count = 0;
    for (const auto &[id, entry] : entries_) {
        if (!entry.finished) {
            ++count;
        }
    }
    return count;
}

void UnitAnimator::removeFinished() {
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (it->second.finished) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace engine
