#pragma once

#include "game/Faction.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace game {

class GameState;

/// Type of action produced by the neutral AI during turn resolution.
enum class AIActionType : std::uint8_t {
    Hold,   ///< Unit stays in place and guards its position.
    Attack, ///< Unit intends to attack an enemy (combat intent, not resolved).
};

/// An action intent produced by the NeutralAI for a single unit.
///
/// For Hold actions, targetRow/targetCol equal the unit's own position.
/// For Attack actions, they indicate the target unit's position.
struct AIAction {
    AIActionType type;

    /// Index of the acting unit in GameState::units().
    std::size_t unitIndex;

    /// Faction that owns the acting unit.
    FactionId actingFactionId;

    /// Position of the target (for Attack intents) or the unit itself (for Hold).
    int targetRow;
    int targetCol;
};

/// Generates actions for neutral factions (hostile and passive) each turn.
///
/// Hostile neutrals scan adjacent tiles for enemy units and produce Attack
/// intents; otherwise they Hold.  Passive neutrals always Hold unless they
/// have been provoked (their diplomatic state changed to War), at which
/// point they behave like hostile neutrals toward the aggressor.
///
/// This class is stateless — all information comes from the GameState.
class NeutralAI {
  public:
    /// Generate all neutral-faction actions for the current turn.
    [[nodiscard]] static std::vector<AIAction> generateActions(const GameState &state);

    /// Spawn neutral factions with units during game initialization.
    ///
    /// Creates one hostile and one passive neutral faction, registers them
    /// in the FactionRegistry, sets up default diplomacy, and places guard
    /// units on the map.
    static void spawnNeutralFactions(GameState &state);

  private:
    /// Generate actions for a single neutral faction.
    [[nodiscard]] static std::vector<AIAction> generateFactionActions(const GameState &state, FactionId factionId,
                                                                      FactionType factionType);

    /// Check whether any enemy unit is adjacent to the given position.
    /// Returns the index of the first enemy found, or std::nullopt.
    [[nodiscard]] static std::optional<std::size_t> findAdjacentEnemy(const GameState &state, int row, int col,
                                                                      FactionId ownFactionId);

    /// Return the six neighbor coordinates for a hex tile (odd-r offset).
    [[nodiscard]] static std::vector<std::pair<int, int>> hexNeighbors(int row, int col);
};

} // namespace game
