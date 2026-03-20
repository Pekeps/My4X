#include "engine/Transition.h"

#include <algorithm>

namespace engine {

void Transition::start(TransitionDir dir) {
    direction_ = dir;
    progress_ = 0.0F;
    active_ = true;
    complete_ = false;
}

void Transition::update(float dt) {
    if (!active_) {
        return;
    }
    progress_ += dt / FADE_DURATION;
    if (progress_ >= 1.0F) {
        progress_ = 1.0F;
        active_ = false;
        complete_ = true;
    }
}

void Transition::draw(int screenWidth, int screenHeight) const {
    float a = alpha();
    if (a <= 0.0F) {
        return;
    }
    auto alphaByte = static_cast<unsigned char>(a * static_cast<float>(MAX_ALPHA));
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, alphaByte});
}

bool Transition::isActive() const { return active_; }

bool Transition::isComplete() const { return complete_; }

float Transition::alpha() const {
    if (direction_ == TransitionDir::FadeOut) {
        return std::clamp(progress_, 0.0F, 1.0F);
    }
    // FadeIn: alpha goes from 1 to 0.
    return std::clamp(1.0F - progress_, 0.0F, 1.0F);
}

} // namespace engine
