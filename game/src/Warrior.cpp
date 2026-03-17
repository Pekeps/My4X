#include "game/Warrior.h"

namespace game {

Warrior::Warrior(int row, int col) : Unit(row, col, DEFAULT_HEALTH, DEFAULT_MOVEMENT, "Warrior") {}

int Warrior::attackPower() const { return attackPower_; }

} // namespace game
