#include "game/Warrior.h"

namespace game {

Warrior::Warrior(int row, int col, const UnitTypeRegistry &registry)
    : Unit(row, col, registry.getTemplate("Warrior")) {}

} // namespace game
