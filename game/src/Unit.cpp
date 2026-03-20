#include "game/Unit.h"

#include <algorithm>
#include <cassert>

namespace game {

Unit::Unit(int row, int col, const UnitTemplate &tmpl, FactionId factionId)
    : row_(row), col_(col), health_(tmpl.maxHealth), movementRemaining_(tmpl.movementRange), factionId_(factionId),
      template_(tmpl) {}

int Unit::row() const { return row_; }
int Unit::col() const { return col_; }

void Unit::moveTo(int row, int col) {
    assert(movementRemaining_ > 0);
    row_ = row;
    col_ = col;
    --movementRemaining_;
}

int Unit::health() const { return health_; }
int Unit::maxHealth() const { return template_.maxHealth; }
bool Unit::isAlive() const { return health_ > 0; }

void Unit::takeDamage(int amount) {
    assert(amount >= 0);
    health_ = std::max(0, health_ - amount);
}

int Unit::attack() const { return template_.attack; }
int Unit::defense() const { return template_.defense; }
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

} // namespace game
