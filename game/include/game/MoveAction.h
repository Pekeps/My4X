#pragma once

#include "game/GameState.h"

#include <cstddef>
#include <cstdint>

namespace game {

/// Validation result for a move action.
enum class MoveValidation : std::uint8_t {
    Valid,
    UnitNotFound,
    UnitDead,
    NoMovementRemaining,
    NotAdjacent,
    ImpassableTerrain,
    InsufficientMovement,
    DestinationOccupied,
    BlockedByZoneOfControl,
};

/// Result of executing a move action.
struct MoveResult {
    bool executed = false;
    MoveValidation validation = MoveValidation::Valid;
    /// Movement cost deducted for the move (terrain-dependent).
    int movementCostDeducted = 0;
};

/// Player-facing move action: move a unit to an adjacent tile, paying the
/// terrain-dependent movement cost.
///
/// Usage:
///   1. Construct with unit index and destination coordinates.
///   2. Call validate() to check preconditions.
///   3. Call execute() to perform the move and deduct movement points.
class MoveAction {
  public:
    /// Construct a move action for the given unit to the destination tile.
    MoveAction(std::size_t unitIndex, int destRow, int destCol);

    /// Validate whether this move can be executed against the current game state.
    [[nodiscard]] MoveValidation validate(const GameState &state) const;

    /// Execute the move: update position, deduct terrain movement cost,
    /// and update the tile registry.
    ///
    /// Returns a MoveResult describing what happened. If validation fails,
    /// the result will have executed=false and the appropriate validation code.
    [[nodiscard]] MoveResult execute(GameState &state) const;

    /// Return the index of the unit being moved.
    [[nodiscard]] std::size_t unitIndex() const;

  private:
    std::size_t unitIndex_;
    int destRow_;
    int destCol_;
};

} // namespace game
