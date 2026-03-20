#include "engine/CombatEffects.h"

#include "engine/HexGrid.h"

#include "raylib.h"

#include <algorithm>
#include <string>

namespace engine {

// ── Spawn methods ────────────────────────────────────────────────────────────

void CombatEffectManager::spawnDamageNumber(float worldX, float worldY, float worldZ, int damage, Color color) {
    damageNumbers_.push_back(FloatingDamageNumber{
        .worldX = worldX,
        .worldY = worldY,
        .worldZ = worldZ,
        .damage = damage,
        .timeRemaining = DAMAGE_NUMBER_DURATION,
        .color = color,
    });
}

void CombatEffectManager::spawnHitFlash(int row, int col) {
    hitFlashes_.push_back(HitFlash{
        .row = row,
        .col = col,
        .timeRemaining = HIT_FLASH_DURATION,
    });
}

void CombatEffectManager::spawnDeathEffect(float worldX, float worldY, float worldZ, Color color) {
    deathEffects_.push_back(DeathEffect{
        .worldX = worldX,
        .worldY = worldY,
        .worldZ = worldZ,
        .timeRemaining = DEATH_EFFECT_DURATION,
        .color = color,
    });
}

// ── Update ───────────────────────────────────────────────────────────────────

void CombatEffectManager::update(float deltaTime) {
    // Update and rise damage numbers.
    for (auto &dmg : damageNumbers_) {
        dmg.timeRemaining -= deltaTime;
        dmg.worldY += DAMAGE_NUMBER_RISE_SPEED * deltaTime;
    }

    // Update hit flashes.
    for (auto &flash : hitFlashes_) {
        flash.timeRemaining -= deltaTime;
    }

    // Update death effects.
    for (auto &death : deathEffects_) {
        death.timeRemaining -= deltaTime;
    }

    // Remove expired effects.
    auto removeExpired = [](auto &vec) {
        vec.erase(std::remove_if(vec.begin(), vec.end(), [](const auto &e) { return e.timeRemaining <= 0.0F; }),
                  vec.end());
    };

    removeExpired(damageNumbers_);
    removeExpired(hitFlashes_);
    removeExpired(deathEffects_);
}

// ── Draw damage numbers (2D overlay) ─────────────────────────────────────────

void CombatEffectManager::drawDamageNumbers(Camera3D camera) const {
    for (const auto &dmg : damageNumbers_) {
        // Project 3D world position to 2D screen position.
        Vector3 worldPos = {dmg.worldX, dmg.worldY, dmg.worldZ};
        Vector2 screenPos = GetWorldToScreen(worldPos, camera);

        // Calculate alpha based on remaining time (fade out).
        float alpha = dmg.timeRemaining / DAMAGE_NUMBER_DURATION;
        auto alphaValue = static_cast<unsigned char>(alpha * DAMAGE_NUMBER_MAX_ALPHA);

        Color textColor = {dmg.color.r, dmg.color.g, dmg.color.b, alphaValue};

        std::string text = std::to_string(dmg.damage);
        int textWidth = MeasureText(text.c_str(), DAMAGE_NUMBER_FONT_SIZE);

        // Center the text horizontally on the projected position.
        int drawX = static_cast<int>(screenPos.x) - (textWidth / 2);
        int drawY = static_cast<int>(screenPos.y);

        DrawText(text.c_str(), drawX, drawY, DAMAGE_NUMBER_FONT_SIZE, textColor);
    }
}

// ── Draw hit flashes (3D) ────────────────────────────────────────────────────

void CombatEffectManager::drawHitFlashes() const {
    for (const auto &flash : hitFlashes_) {
        Vector3 center = hex::tileCenter(flash.row, flash.col);

        float alpha = flash.timeRemaining / HIT_FLASH_DURATION;
        static constexpr float HIT_FLASH_MAX_ALPHA = 180.0F;
        static constexpr float HIT_FLASH_SPHERE_RADIUS = 0.6F;
        auto alphaValue = static_cast<unsigned char>(alpha * HIT_FLASH_MAX_ALPHA);

        static constexpr unsigned char HIT_FLASH_RED = 255;
        static constexpr unsigned char HIT_FLASH_GREEN = 60;
        static constexpr unsigned char HIT_FLASH_BLUE = 60;
        Color flashColor = {HIT_FLASH_RED, HIT_FLASH_GREEN, HIT_FLASH_BLUE, alphaValue};
        DrawSphere(center, HIT_FLASH_SPHERE_RADIUS, flashColor);
    }
}

// ── Draw death effects (3D) ──────────────────────────────────────────────────

void CombatEffectManager::drawDeathEffects() const {
    for (const auto &death : deathEffects_) {
        float progress = 1.0F - (death.timeRemaining / DEATH_EFFECT_DURATION);
        float scale = DEATH_EFFECT_START_SCALE - (progress * (DEATH_EFFECT_START_SCALE - DEATH_EFFECT_END_SCALE));
        float alpha = (1.0F - progress) * DEATH_EFFECT_MAX_ALPHA;

        auto alphaValue = static_cast<unsigned char>(alpha);
        Color effectColor = {death.color.r, death.color.g, death.color.b, alphaValue};

        float radius = DEATH_EFFECT_RADIUS * scale;
        Vector3 pos = {death.worldX, death.worldY, death.worldZ};

        if (radius > 0.0F) {
            DrawSphere(pos, radius, effectColor);
        }
    }
}

// ── Query methods ────────────────────────────────────────────────────────────

std::size_t CombatEffectManager::activeDamageNumberCount() const { return damageNumbers_.size(); }

std::size_t CombatEffectManager::activeHitFlashCount() const { return hitFlashes_.size(); }

std::size_t CombatEffectManager::activeDeathEffectCount() const { return deathEffects_.size(); }

} // namespace engine
