#include "game/NeutralAI.h"

#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/Unit.h"
#include "game/UnitTemplate.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <array>
#include <optional>
#include <utility>

namespace game {

namespace {

// ── Spawning constants ───────────────────────────────────────────────────────

/// Color indices for neutral factions.
constexpr std::uint8_t HOSTILE_COLOR_INDEX = 6;
constexpr std::uint8_t PASSIVE_COLOR_INDEX = 7;

/// Starting positions for hostile neutral guards (row, col).
constexpr std::array<std::pair<int, int>, 3> HOSTILE_POSITIONS = {{
    {2, 2},
    {2, 3},
    {3, 2},
}};

/// Starting positions for passive neutral guards (row, col).
constexpr std::array<std::pair<int, int>, 2> PASSIVE_POSITIONS = {{
    {7, 7},
    {7, 8},
}};

} // namespace

std::vector<AIAction> NeutralAI::generateActions(const GameState &state) {
    std::vector<AIAction> actions;

    for (const Faction &faction : state.factionRegistry().allFactions()) {
        if (faction.type() == FactionType::Player) {
            continue;
        }

        auto factionActions = generateFactionActions(state, faction.id(), faction.type());
        actions.insert(actions.end(), factionActions.begin(), factionActions.end());
    }

    return actions;
}

std::vector<AIAction> NeutralAI::generateFactionActions(const GameState &state, FactionId factionId,
                                                        FactionType /*factionType*/) {
    std::vector<AIAction> actions;

    const auto &units = state.units();
    for (std::size_t i = 0; i < units.size(); ++i) {
        const auto &unit = units[i];
        if (unit->factionId() != factionId || !unit->isAlive()) {
            continue;
        }

        // Both hostile and passive neutrals scan for adjacent enemies.
        // The diplomacy check inside findAdjacentEnemy ensures that:
        //   - Hostile neutrals find enemies they are at war with (players, passive neutrals).
        //   - Passive neutrals only find enemies if their relation has been set to War
        //     (i.e., they were provoked); otherwise findAdjacentEnemy returns nullopt.
        auto enemyIdx = findAdjacentEnemy(state, unit->row(), unit->col(), factionId);
        if (enemyIdx.has_value()) {
            const auto &enemy = units[*enemyIdx];
            actions.push_back(AIAction{
                .type = AIActionType::Attack,
                .unitIndex = i,
                .actingFactionId = factionId,
                .targetRow = enemy->row(),
                .targetCol = enemy->col(),
            });
            continue;
        }

        // Default: hold position.
        actions.push_back(AIAction{
            .type = AIActionType::Hold,
            .unitIndex = i,
            .actingFactionId = factionId,
            .targetRow = unit->row(),
            .targetCol = unit->col(),
        });
    }

    return actions;
}

std::optional<std::size_t> NeutralAI::findAdjacentEnemy(const GameState &state, int row, int col,
                                                        FactionId ownFactionId) {
    auto neighbors = hexNeighbors(row, col);
    const auto &units = state.units();
    const auto &diplomacy = state.diplomacy();

    for (const auto &[nRow, nCol] : neighbors) {
        // Check bounds.
        if (nRow < 0 || nCol < 0 || nRow >= state.map().height() || nCol >= state.map().width()) {
            continue;
        }

        // Check all units on this neighboring tile.
        for (std::size_t i = 0; i < units.size(); ++i) {
            const auto &unit = units[i];
            if (unit->row() == nRow && unit->col() == nCol && unit->isAlive() && unit->factionId() != ownFactionId &&
                diplomacy.areAtWar(ownFactionId, unit->factionId())) {
                return i;
            }
        }
    }

    return std::nullopt;
}

std::vector<std::pair<int, int>> NeutralAI::hexNeighbors(int row, int col) {
    // Odd-r offset coordinates: odd rows are shifted right.
    std::vector<std::pair<int, int>> result;
    static constexpr int NEIGHBOR_COUNT = 6;
    result.reserve(NEIGHBOR_COUNT);

    if (row % 2 == 0) {
        // Even row
        result.emplace_back(row - 1, col - 1);
        result.emplace_back(row - 1, col);
        result.emplace_back(row, col - 1);
        result.emplace_back(row, col + 1);
        result.emplace_back(row + 1, col - 1);
        result.emplace_back(row + 1, col);
    } else {
        // Odd row
        result.emplace_back(row - 1, col);
        result.emplace_back(row - 1, col + 1);
        result.emplace_back(row, col - 1);
        result.emplace_back(row, col + 1);
        result.emplace_back(row + 1, col);
        result.emplace_back(row + 1, col + 1);
    }

    return result;
}

void NeutralAI::spawnNeutralFactions(GameState &state) {
    auto &registry = state.mutableFactionRegistry();

    // Create hostile neutral faction.
    FactionId hostileId = registry.addFaction("Barbarians", FactionType::NeutralHostile, HOSTILE_COLOR_INDEX);

    // Create passive neutral faction.
    FactionId passiveId = registry.addFaction("City-State", FactionType::NeutralPassive, PASSIVE_COLOR_INDEX);

    // Initialize diplomacy defaults for all factions.
    state.mutableDiplomacy().initializeDefaults(registry);

    // Spawn hostile guard units.
    UnitTypeRegistry unitReg;
    unitReg.registerDefaults();

    for (const auto &[row, col] : HOSTILE_POSITIONS) {
        // Clamp to map bounds.
        if (row < state.map().height() && col < state.map().width()) {
            state.addUnit(std::make_unique<Warrior>(row, col, unitReg, hostileId));
        }
    }

    // Spawn passive guard units.
    for (const auto &[row, col] : PASSIVE_POSITIONS) {
        // Clamp to map bounds.
        if (row < state.map().height() && col < state.map().width()) {
            state.addUnit(std::make_unique<Warrior>(row, col, unitReg, passiveId));
        }
    }
}

} // namespace game
