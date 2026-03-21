#pragma once

#include "game/Faction.h"
#include "game/Resource.h"
#include "game/TerrainType.h"
#include "game/TileCoord.h"

#include <cstdint>
#include <set>
#include <string>

namespace game {

using BuildingId = std::uint32_t;

/// A building that occupies one or more hex tiles and produces resources.
///
/// Buildings are data-driven: different building "types" are just instances
/// with different stats. Factory functions create common configurations.
class Building {
  public:
    Building(std::string name, Resource yieldPerTurn, Resource constructionCost, std::set<TileCoord> occupiedTiles,
             std::set<TerrainType> allowedTerrains = {}, std::string modelKey = "", FactionId factionId = 0,
             bool underConstruction = false);

    [[nodiscard]] BuildingId id() const;
    void setId(BuildingId newId);

    [[nodiscard]] const std::string &name() const;
    [[nodiscard]] const Resource &yieldPerTurn() const;
    [[nodiscard]] const Resource &constructionCost() const;
    [[nodiscard]] const std::set<TileCoord> &occupiedTiles() const;
    [[nodiscard]] const std::set<TerrainType> &allowedTerrains() const;

    /// Key used to look up this building type's 3D model in ModelManager.
    /// When empty, the renderer falls back to a generic placeholder.
    [[nodiscard]] const std::string &modelKey() const;

    /// Faction that owns this building (0 = unowned / legacy).
    [[nodiscard]] FactionId factionId() const;
    void setFactionId(FactionId fid);

    /// Whether this building is still being constructed.
    /// Under-construction buildings are rendered at reduced scale.
    [[nodiscard]] bool underConstruction() const;
    void setUnderConstruction(bool value);

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
    std::string modelKey_;
    FactionId factionId_ = 0;
    bool underConstruction_ = false;
};

// ── Factory functions for common building types ──────────────────────────
// Signature is (int row, int col) to stay compatible with BuildingFactory.
// Use setFactionId() on the returned building to assign ownership.

/// City center — the main building at a city's heart, larger cube.
[[nodiscard]] Building makeCityCenter(int row, int col);

/// Farm — produces food, single tile.
[[nodiscard]] Building makeFarm(int row, int col);

/// Mine — produces production, single tile, restricted to Hills.
[[nodiscard]] Building makeMine(int row, int col);

/// Lumber mill — produces production, single tile, restricted to Forest.
[[nodiscard]] Building makeLumberMill(int row, int col);

/// Barracks — produces nothing, enables unit training, single tile.
[[nodiscard]] Building makeBarracks(int row, int col);

/// Market — produces gold, single tile.
[[nodiscard]] Building makeMarket(int row, int col);

} // namespace game
