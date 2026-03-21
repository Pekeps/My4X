#include "game/TurnResolver.h"

#include "game/BuildQueue.h"
#include "game/Building.h"
#include "game/City.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/NeutralAI.h"
#include "game/Resource.h"
#include "game/UnitTypeRegistry.h"

#include <array>
#include <memory>
#include <variant>
#include <vector>

namespace game {

namespace {

/// Number of hex neighbors for any tile.
constexpr int kHexNeighborCount = 6;

/// A (row-delta, col-delta) pair for hex neighbor offsets.
struct HexOffset {
    int dr;
    int dc;
};

/// Odd-r offset hex neighbor directions for even rows (row % 2 == 0).
constexpr std::array<HexOffset, kHexNeighborCount> kEvenRowOffsets = {{
    {.dr = -1, .dc = -1},
    {.dr = -1, .dc = 0},
    {.dr = 0, .dc = 1},
    {.dr = 1, .dc = 0},
    {.dr = 1, .dc = -1},
    {.dr = 0, .dc = -1},
}};

/// Odd-r offset hex neighbor directions for odd rows (row % 2 != 0).
constexpr std::array<HexOffset, kHexNeighborCount> kOddRowOffsets = {{
    {.dr = -1, .dc = 0},
    {.dr = -1, .dc = 1},
    {.dr = 0, .dc = 1},
    {.dr = 1, .dc = 1},
    {.dr = 1, .dc = 0},
    {.dr = 0, .dc = -1},
}};

/// Return all valid neighbor coordinates for a hex tile in odd-r offset layout.
std::vector<std::pair<int, int>> hexNeighbors(int row, int col, int mapRows, int mapCols) {
    std::vector<std::pair<int, int>> result;
    result.reserve(kHexNeighborCount);

    const bool isOddRow = (row % 2 != 0);
    const auto &offsets = isOddRow ? kOddRowOffsets : kEvenRowOffsets;

    for (const auto &offset : offsets) {
        int nr = row + offset.dr;
        int nc = col + offset.dc;
        if (nr >= 0 && nr < mapRows && nc >= 0 && nc < mapCols) {
            result.emplace_back(nr, nc);
        }
    }
    return result;
}

/// Find a free tile to spawn a unit near a city center.
/// Prefers adjacent tiles first; falls back to the city center tile.
std::pair<int, int> findSpawnTile(int centerRow, int centerCol, const GameState &state) {
    int mapRows = state.map().height();
    int mapCols = state.map().width();

    // Try each adjacent tile — pick the first one with no units.
    auto neighbors = hexNeighbors(centerRow, centerCol, mapRows, mapCols);
    for (const auto &[nr, nc] : neighbors) {
        if (state.registry().unitsAt(nr, nc).empty()) {
            return {nr, nc};
        }
    }

    // Fallback: spawn on the city center itself.
    return {centerRow, centerCol};
}

/// Sum the resource yields of all buildings whose tiles overlap the city.
Resource collectCityYields(const City &city, const GameState &state) {
    Resource total;
    for (const Building &building : state.buildings()) {
        for (const TileCoord &coord : building.occupiedTiles()) {
            if (city.containsTile(coord.row, coord.col)) {
                total += building.yieldPerTurn();
                break; // count each building once
            }
        }
    }
    return total;
}

/// Route gold yields to the owning faction's stockpile.
/// If the faction exists in the registry, gold goes there;
/// otherwise it falls back to the global factionResources().
void routeGoldToFaction(GameState &state, int factionId, int gold) {
    if (factionId > 0) {
        auto targetId = static_cast<FactionId>(factionId);
        for (Faction &faction : state.mutableFactionRegistry().allMutableFactions()) {
            if (faction.id() == targetId) {
                faction.stockpile().gold += gold;
                return;
            }
        }
    }
    state.factionResources().gold += gold;
}

/// Spawn a unit from a completed ProduceUnitOrder.
void spawnCompletedUnit(GameState &state, const City &city, const UnitSpawnRequest &request,
                        const UnitTypeRegistry &unitRegistry) {
    const UnitTemplate &tmpl = unitRegistry.getTemplate(request.templateKey);
    auto [spawnRow, spawnCol] = findSpawnTile(city.centerRow(), city.centerCol(), state);
    auto unit = std::make_unique<Unit>(spawnRow, spawnCol, tmpl, static_cast<FactionId>(city.factionId()));
    state.addUnit(std::move(unit));
}

/// Handle a completed build queue result — place building or spawn unit.
void handleCompletedProduction(GameState &state, const City &city, BuildQueueResult &result,
                               const UnitTypeRegistry *unitRegistry) {
    if (std::holds_alternative<Building>(result)) {
        auto &building = std::get<Building>(result);
        building.setFactionId(static_cast<FactionId>(city.factionId()));
        state.addBuilding(std::move(building));
    } else if (unitRegistry != nullptr) {
        spawnCompletedUnit(state, city, std::get<UnitSpawnRequest>(result), *unitRegistry);
    }
}

/// Core turn resolution: yields, production, food, for all cities.
void resolveCities(GameState &state, const UnitTypeRegistry *unitRegistry) {
    for (City &city : state.mutableCities()) {
        Resource yields = collectCityYields(city, state);

        // Gold goes to the owning faction's stockpile.
        routeGoldToFaction(state, city.factionId(), yields.gold);

        // Production: base city production + building production yields.
        int totalProduction = City::productionPerTurn() + yields.production;
        auto completed = city.buildQueue().applyProduction(totalProduction);
        if (completed.has_value()) {
            handleCompletedProduction(state, city, *completed, unitRegistry);
        }

        // Food: accumulate surplus, grow population when threshold is met.
        city.addFoodSurplus(yields.food);
        int threshold = City::growthThreshold(city.population());
        if (city.foodSurplus() >= threshold) {
            city.growPopulation(1);
            city.setFoodSurplus(city.foodSurplus() - threshold);
        }
    }
}

} // namespace

void resolveTurn(GameState &state) {
    // 1. Collect yields and apply production / food for each city.
    resolveCities(state, nullptr);

    // 2. Generate neutral AI actions (attack intents, holds).
    //    Actions are generated but not resolved — combat is Phase 3.
    [[maybe_unused]] auto aiActions = NeutralAI::generateActions(state);

    // 3. Reset unit movement.
    for (auto &unit : state.units()) {
        unit->resetMovement();
    }

    // 4. Advance the turn counter.
    state.nextTurn();
}

void resolveTurn(GameState &state, std::vector<AIAction> &outActions) {
    // 1. Collect yields and apply production / food for each city.
    resolveCities(state, nullptr);

    // 2. Generate neutral AI actions (attack intents, holds).
    outActions = NeutralAI::generateActions(state);

    // 3. Reset unit movement.
    for (auto &unit : state.units()) {
        unit->resetMovement();
    }

    // 4. Advance the turn counter.
    state.nextTurn();
}

void resolveTurn(GameState &state, const UnitTypeRegistry *unitRegistry) {
    // 1. Collect yields and apply production / food for each city.
    resolveCities(state, unitRegistry);

    // 2. Generate neutral AI actions (attack intents, holds).
    //    Actions are generated but not resolved — combat is Phase 3.
    [[maybe_unused]] auto aiActions = NeutralAI::generateActions(state);

    // 3. Reset unit movement.
    for (auto &unit : state.units()) {
        unit->resetMovement();
    }

    // 4. Advance the turn counter.
    state.nextTurn();
}

} // namespace game
