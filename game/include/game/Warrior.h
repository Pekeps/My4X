#pragma once

#include "game/Unit.h"

namespace game {

class Warrior : public Unit {
  public:
    Warrior(int row, int col);

    [[nodiscard]] int attackPower() const;

  private:
    static const int DEFAULT_HEALTH = 100;
    static const int DEFAULT_MOVEMENT = 2;
    static const int DEFAULT_ATTACK = 15;

    int attackPower_ = DEFAULT_ATTACK;
};

} // namespace game
