#include "game/Unit.h"

#include <algorithm>
#include <cassert>

namespace game {

Unit::Unit(int row, int col, const UnitTemplate &tmpl, FactionId factionId)
    : row_(row), col_(col), health_(tmpl.maxHealth), movementRemaining_(tmpl.movementRange), factionId_(factionId),
      template_(tmpl) {}

int Unit::row() const { return row_; }
int Unit::col() const { return col_; }

void Unit::moveTo(int row, int col, int cost) {
    assert(cost > 0);
    assert(movementRemaining_ >= cost);
    row_ = row;
    col_ = col;
    movementRemaining_ -= cost;
}

int Unit::health() const { return health_; }
int Unit::maxHealth() const { return template_.maxHealth; }
bool Unit::isAlive() const { return health_ > 0; }

void Unit::takeDamage(int amount) {
    assert(amount >= 0);
    health_ = std::max(0, health_ - amount);
}

int Unit::baseAttack() const { return template_.attack; }
int Unit::baseDefense() const { return template_.defense; }

int Unit::attack() const { return template_.attack + (level_ * STAT_BONUS_PER_LEVEL); }
int Unit::defense() const { return template_.defense + (level_ * STAT_BONUS_PER_LEVEL); }
int Unit::attackRange() const { return template_.attackRange; }

int Unit::movement() const { return template_.movementRange; }
int Unit::movementRemaining() const { return movementRemaining_; }
void Unit::resetMovement() { movementRemaining_ = template_.movementRange; }
void Unit::setHealth(int health) { health_ = std::max(0, std::min(health, template_.maxHealth)); }
void Unit::setMovementRemaining(int remaining) {
    movementRemaining_ = std::max(0, std::min(remaining, template_.movementRange));
}

const std::string &Unit::name() const { return template_.name; }

FactionId Unit::factionId() const { return factionId_; }

const UnitTemplate &Unit::unitTemplate() const { return template_; }

// ── Experience / leveling ────────────────────────────────────────────────────

int Unit::experience() const { return experience_; }

int Unit::level() const { return level_; }

bool Unit::addExperience(int xp) {
    if (xp <= 0) {
        return false;
    }
    int previousLevel = level_;
    experience_ += xp;
    recomputeLevel();
    if (level_ > previousLevel) {
        justLeveledUp_ = true;
        return true;
    }
    return false;
}

void Unit::setExperience(int xp) {
    experience_ = std::max(0, xp);
    recomputeLevel();
}

bool Unit::justLeveledUp() const { return justLeveledUp_; }

void Unit::clearLevelUpFlag() { justLeveledUp_ = false; }

void Unit::recomputeLevel() {
    int newLevel = 0;
    for (int i = 0; i < XP_THRESHOLD_COUNT; ++i) {
        if (experience_ >= XP_THRESHOLDS.at(i)) {
            newLevel = i + 1;
        } else {
            break;
        }
    }
    level_ = std::min(newLevel, MAX_UNIT_LEVEL);
}

} // namespace game
