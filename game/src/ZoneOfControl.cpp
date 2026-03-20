#include "game/ZoneOfControl.h"

#include "game/Pathfinding.h"
#include "game/Unit.h"

#include <algorithm>
#include <ranges>
#include <vector>

namespace game::zoc {

namespace {

/// Collect the set of enemy faction IDs whose units are adjacent to (row, col).
/// Only considers factions at war with the given factionId.
std::vector<FactionId> findAdjacentEnemyFactions(int row, int col, FactionId factionId, const Map &map,
                                                 const TileRegistry &registry, const DiplomacyManager &diplomacy) {
    std::vector<FactionId> result;

    auto neighbors = hexNeighbors(row, col, map.height(), map.width());

    for (const auto &neighbor : neighbors) {
        auto units = registry.unitsAt(neighbor.row, neighbor.col);

        for (const auto *unit : units) {
            if (unit->factionId() == factionId) {
                continue;
            }
            if (!diplomacy.areAtWar(factionId, unit->factionId())) {
                continue;
            }
            // Avoid duplicates: only add if not already present.
            if (std::ranges::find(result, unit->factionId()) == result.end()) {
                result.push_back(unit->factionId());
            }
        }
    }

    return result;
}

} // namespace

bool isInEnemyZoc(int row, int col, FactionId factionId, const Map &map, const TileRegistry &registry,
                  const DiplomacyManager &diplomacy) {
    return !findAdjacentEnemyFactions(row, col, factionId, map, registry, diplomacy).empty();
}

bool isMovementBlockedByZoc(int fromRow, int fromCol, int toRow, int toCol, FactionId factionId, const Map &map,
                            const TileRegistry &registry, const DiplomacyManager &diplomacy) {
    // Get enemy factions exerting ZoC over the destination.
    auto destEnemies = findAdjacentEnemyFactions(toRow, toCol, factionId, map, registry, diplomacy);

    if (destEnemies.empty()) {
        // Destination is not in any enemy ZoC — move is never blocked by ZoC.
        return false;
    }

    // Get enemy factions exerting ZoC over the source.
    auto srcEnemies = findAdjacentEnemyFactions(fromRow, fromCol, factionId, map, registry, diplomacy);

    // The move is blocked if any enemy faction exerts ZoC over *both* tiles
    // (i.e., the unit would move directly from one ZoC hex to another of the
    // same enemy).
    return std::ranges::any_of(destEnemies, [&srcEnemies](FactionId enemy) {
        return std::ranges::find(srcEnemies, enemy) != srcEnemies.end();
    });
}

std::vector<FactionId> enemyZocSources(int row, int col, FactionId factionId, const Map &map,
                                       const TileRegistry &registry, const DiplomacyManager &diplomacy) {
    return findAdjacentEnemyFactions(row, col, factionId, map, registry, diplomacy);
}

} // namespace game::zoc
