#pragma once

#include "game/City.h"
#include "game/Faction.h"
#include "game/Map.h"
#include "game/Unit.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace game {

/// Visibility state of a tile from a particular faction's perspective.
enum class VisibilityState : std::uint8_t {
    /// Never seen by any unit or city of this faction.
    Unexplored,
    /// Previously visible but no longer in sight range.
    Explored,
    /// Currently within sight range of a friendly unit or city.
    Visible,
};

/// Fixed sight range for cities (measured from each city tile, in hex distance).
static constexpr int CITY_SIGHT_RANGE = 3;

/// Tracks per-faction tile visibility for the fog of war system.
///
/// Each faction has its own 2D grid of VisibilityState values. Visibility is
/// recalculated from scratch whenever recalculate() is called (typically at
/// turn start and after unit movement). Tiles transition:
///   Unexplored -> Visible  (first time in sight range)
///   Visible    -> Explored (was visible, now out of range)
///   Explored   -> Visible  (re-enters sight range)
///
/// This is a pure game-layer class with no rendering dependencies.
class FogOfWar {
  public:
    /// Construct a fog of war tracker for a map of the given dimensions.
    FogOfWar(int rows, int cols);

    /// Return the visibility of a tile for the given faction.
    /// Returns Unexplored if the faction has no visibility grid yet.
    [[nodiscard]] VisibilityState getVisibility(FactionId factionId, int row, int col) const;

    /// Recompute visibility for the given faction from scratch.
    ///
    /// All tiles currently marked Visible are demoted to Explored (preserving
    /// previously-explored status). Then every hex within sight range of a
    /// friendly unit or city tile is marked Visible.
    void recalculate(FactionId factionId, const std::vector<std::unique_ptr<Unit>> &units,
                     const std::vector<City> &cities, const Map &map);

    /// Return the map dimensions this FogOfWar was created for.
    [[nodiscard]] int rows() const;
    [[nodiscard]] int cols() const;

    /// Compute hex distance between two positions using odd-r offset coordinates.
    /// Exposed as a static utility so tests and other systems can reuse it.
    [[nodiscard]] static int hexDistance(int row1, int col1, int row2, int col2);

  private:
    /// Ensure a visibility grid exists for the given faction, initializing it
    /// to all-Unexplored if needed. Returns a reference to the grid.
    std::vector<VisibilityState> &ensureGrid(FactionId factionId);

    /// Look up an existing grid. Returns nullptr if the faction has no grid.
    [[nodiscard]] const std::vector<VisibilityState> *findGrid(FactionId factionId) const;

    /// Convert (row, col) to a linear index into the visibility grid.
    [[nodiscard]] std::size_t index(int row, int col) const;

    /// Mark all tiles within the given sight range of (centerRow, centerCol) as
    /// Visible in the provided grid, respecting map bounds.
    void applyVision(std::vector<VisibilityState> &grid, int centerRow, int centerCol, int sightRange,
                     const Map &map) const;

    int rows_;
    int cols_;

    /// Per-faction visibility grids, keyed by FactionId.
    /// Each grid is a flat vector of size rows_ * cols_.
    std::unordered_map<FactionId, std::vector<VisibilityState>> grids_;
};

} // namespace game
