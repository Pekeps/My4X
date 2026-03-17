#pragma once

#include "game/Resource.h"
#include "game/TerrainType.h"

#include <cstdint>
#include <set>
#include <string>

namespace game {

using BuildingId = std::uint32_t;

/// Coordinate pair for a hex tile.
struct TileCoord {
    int row;
    int col;

    [[nodiscard]] friend bool operator==(const TileCoord &lhs, const TileCoord &rhs) = default;
    [[nodiscard]] friend auto operator<=>(const TileCoord &lhs, const TileCoord &rhs) = default;
};

/// A building that occupies one or more hex tiles and produces resources.
///
/// Buildings are data-driven: different building "types" are just instances
/// with different stats. Factory functions (makeFarm, makeMine, makeMarket)
/// create common configurations.
class Building {
  public:
    Building(std::string name, Resource yieldPerTurn, Resource constructionCost, std::set<TileCoord> occupiedTiles,
             std::set<TerrainType> allowedTerrains = {});

    [[nodiscard]] BuildingId id() const;
    void setId(BuildingId newId);

    [[nodiscard]] const std::string &name() const;
    [[nodiscard]] const Resource &yieldPerTurn() const;
    [[nodiscard]] const Resource &constructionCost() const;
    [[nodiscard]] const std::set<TileCoord> &occupiedTiles() const;
    [[nodiscard]] const std::set<TerrainType> &allowedTerrains() const;

    /// Returns true if the building has no terrain restrictions
    /// or the given terrain is in the allowed set.
    [[nodiscard]] bool canBuildOn(TerrainType terrain) const;

  private:
    BuildingId id_ = 0;
    std::string name_;
    Resource yieldPerTurn_;
    Resource constructionCost_;
    std::set<TileCoord> occupiedTiles_;
    std::set<TerrainType> allowedTerrains_;
};

// ── Factory functions for common building types ──────────────────────────

/// Farm — produces food, single tile.
[[nodiscard]] Building makeFarm(int row, int col);

/// Mine — produces production, single tile, restricted to Hills.
[[nodiscard]] Building makeMine(int row, int col);

/// Market — produces gold, single tile.
[[nodiscard]] Building makeMarket(int row, int col);

} // namespace game
