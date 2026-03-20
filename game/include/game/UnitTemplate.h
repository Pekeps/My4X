#pragma once

#include "game/Resource.h"

#include <string>

namespace game {

/// Data-driven definition of a unit type's stats.
///
/// Each distinct unit type (Warrior, Archer, Settler, etc.) is described by a
/// single UnitTemplate instance. At runtime, Unit objects reference their
/// template for stat lookups rather than storing per-subclass constants.
struct UnitTemplate {
    std::string name;

    int maxHealth = 0;
    int attack = 0;
    int defense = 0;
    int movementRange = 0;

    /// 1 = melee, >1 = ranged (needed for Phase 3 combat).
    int attackRange = 1;

    /// How far this unit can see (in hex tiles). Used by the fog-of-war system.
    int sightRange = 2;

    /// Resource cost to produce this unit in a city build queue.
    Resource productionCost;
};

} // namespace game
