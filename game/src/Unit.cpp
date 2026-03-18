#include "game/Unit.h"

#include <algorithm>
#include <cassert>
#include <utility>

namespace game {

Unit::Unit(int row, int col, int health, int movement, std::string name)
    : row_(row), col_(col), health_(health), maxHealth_(health), movement_(movement), movementRemaining_(movement),
      name_(std::move(name)) {}

int Unit::row() const { return row_; }
int Unit::col() const { return col_; }

void Unit::moveTo(int row, int col) {
    assert(movementRemaining_ > 0);
    row_ = row;
    col_ = col;
    --movementRemaining_;
}

int Unit::health() const { return health_; }
int Unit::maxHealth() const { return maxHealth_; }
bool Unit::isAlive() const { return health_ > 0; }

void Unit::takeDamage(int amount) {
    assert(amount >= 0);
    health_ = std::max(0, health_ - amount);
}

int Unit::movement() const { return movement_; }
int Unit::movementRemaining() const { return movementRemaining_; }
void Unit::resetMovement() { movementRemaining_ = movement_; }
void Unit::setHealth(int health) { health_ = std::max(0, std::min(health, maxHealth_)); }
void Unit::setMovementRemaining(int remaining) { movementRemaining_ = std::max(0, std::min(remaining, movement_)); }

const std::string &Unit::name() const { return name_; }

} // namespace game
