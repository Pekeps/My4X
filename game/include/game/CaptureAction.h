#pragma once

#include "game/City.h"
#include "game/DiplomacyManager.h"
#include "game/Faction.h"
#include "game/GameState.h"
#include "game/Unit.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace game {

/// Validation result for a capture action.
enum class CaptureValidation : std::uint8_t {
    Valid,
    UnitNotFound,
    UnitDead,
    NoCityAtPosition,
    CityOwnedBySameFaction,
    NotAtWar,
    CityIsDefended,
};

/// Result of executing a capture action.
struct CaptureResult {
    bool executed = false;
    CaptureValidation validation = CaptureValidation::Valid;

    /// Name of the captured city (for logging/UI).
    std::string capturedCityName;

    /// The faction that previously owned the city.
    FactionId previousOwner = 0;

    /// The faction that now owns the city.
    FactionId newOwner = 0;

    /// Whether any faction was eliminated as a result of this capture.
    bool factionEliminated = false;

    /// The faction ID that was eliminated (valid only when factionEliminated is true).
    FactionId eliminatedFactionId = 0;
};

/// Captures an enemy city when a unit moves onto its center tile.
///
/// Capture conditions:
///   - The unit is alive and on the center tile of an enemy city.
///   - The unit's faction is at war with the city's faction.
///   - No enemy units occupy any tile of the city.
///
/// On capture:
///   - City ownership transfers to the capturing unit's faction.
///   - Production queue is cleared.
///   - Buildings are preserved.
///   - If the previous owner has no cities and no units remaining, they are eliminated.
class CaptureAction {
  public:
    /// Construct a capture action for the unit at the given index.
    explicit CaptureAction(std::size_t unitIndex);

    /// Validate whether this capture can be executed.
    [[nodiscard]] CaptureValidation validate(const GameState &state) const;

    /// Execute the capture. Returns a result describing what happened.
    [[nodiscard]] CaptureResult execute(GameState &state) const;

    /// Check whether a faction has been eliminated (no cities and no units).
    [[nodiscard]] static bool isFactionEliminated(const GameState &state, FactionId factionId);

    /// Return the index of the capturing unit.
    [[nodiscard]] std::size_t unitIndex() const;

  private:
    std::size_t unitIndex_;

    /// Find the city whose center tile the unit is standing on.
    /// Returns nullptr if no such city exists.
    [[nodiscard]] static City *findCityCenterAt(GameState &state, int row, int col);

    /// Check whether any enemy units (from the city's faction) are on any of
    /// the city's tiles.
    [[nodiscard]] static bool isCityDefended(const GameState &state, const City &city);
};

} // namespace game
