#pragma once

#include "game/DiplomacyManager.h"
#include "game/Faction.h"
#include "game/Map.h"
#include "game/TileCoord.h"
#include "game/TileRegistry.h"

#include <vector>

namespace game {

/// Number of adjacent hexes a unit exerts zone of control over.
static constexpr int ZOC_ADJACENT_HEX_COUNT = 6;

/// Zone-of-control (ZoC) query interface.
///
/// A unit exerts ZoC over all six adjacent hexes. Enemy units that enter
/// a ZoC hex must stop immediately (cannot continue moving through it),
/// and moving directly from one ZoC hex to another ZoC hex of the same
/// enemy is not allowed without retreating first.
///
/// ZoC only applies between factions at war (checked via DiplomacyManager).
/// Friendly and neutral-peace ZoC does not restrict movement.
namespace zoc {

/// Return true if the hex at (row, col) is inside a zone of control that
/// is hostile to the given faction.
///
/// A hex is in hostile ZoC when any unit belonging to a faction at war with
/// @p factionId occupies an adjacent hex.
[[nodiscard]] bool isInEnemyZoc(int row, int col, FactionId factionId, const Map &map, const TileRegistry &registry,
                                const DiplomacyManager &diplomacy);

/// Return true if a move from (fromRow, fromCol) to (toRow, toCol) is
/// forbidden by zone-of-control rules for the given faction.
///
/// A move is ZoC-blocked if the destination hex is in enemy ZoC **and**
/// the source hex is also in ZoC of the same enemy faction (moving
/// directly between two ZoC hexes of the same enemy is prohibited).
///
/// Note: entering an enemy ZoC hex is allowed (the unit just must stop
/// there). This function only blocks the "ZoC to ZoC" transition.
[[nodiscard]] bool isMovementBlockedByZoc(int fromRow, int fromCol, int toRow, int toCol, FactionId factionId,
                                          const Map &map, const TileRegistry &registry,
                                          const DiplomacyManager &diplomacy);

/// Return all enemy faction IDs that exert ZoC over the given hex.
///
/// Useful for determining which specific enemies constrain a tile, e.g.
/// to check shared-enemy ZoC-to-ZoC transitions.
[[nodiscard]] std::vector<FactionId> enemyZocSources(int row, int col, FactionId factionId, const Map &map,
                                                     const TileRegistry &registry, const DiplomacyManager &diplomacy);

} // namespace zoc
} // namespace game
