#include "game/Warrior.h"

namespace game {

Warrior::Warrior(int row, int col, const UnitTypeRegistry &registry, FactionId factionId)
    : Unit(row, col, registry.getTemplate("Warrior"), factionId) {}

} // namespace game
