#pragma once

#include "game/Faction.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"

namespace game {

/// Thin subclass that constructs a Unit with the "Warrior" template.
///
/// Kept for backward compatibility and convenience — concrete subclasses are
/// optional now that stats are template-driven.
class Warrior : public Unit {
  public:
    Warrior(int row, int col, const UnitTypeRegistry &registry, FactionId factionId);
};

} // namespace game
