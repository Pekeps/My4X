#pragma once

#include "game/HexDirection.h"
#include "game/TerrainType.h"

namespace game {

class Tile {
  public:
    Tile(int row, int col, TerrainType terrain = TerrainType::Plains);

    [[nodiscard]] int row() const;
    [[nodiscard]] int col() const;
    [[nodiscard]] TerrainType terrainType() const;
    void setTerrainType(TerrainType terrain);

    [[nodiscard]] int elevation() const;
    void setElevation(int elevation);

    [[nodiscard]] bool hasIncomingRiver() const;
    [[nodiscard]] bool hasOutgoingRiver() const;
    [[nodiscard]] bool hasRiver() const;
    [[nodiscard]] bool hasRiverBeginOrEnd() const;
    [[nodiscard]] bool hasRiverThroughEdge(HexDirection dir) const;
    [[nodiscard]] HexDirection incomingRiverDirection() const;
    [[nodiscard]] HexDirection outgoingRiverDirection() const;

    void setOutgoingRiver(HexDirection dir);
    void removeOutgoingRiver();
    void setIncomingRiver(HexDirection dir);
    void removeIncomingRiver();

    [[nodiscard]] int waterLevel() const;
    void setWaterLevel(int level);
    [[nodiscard]] bool isUnderwater() const;

  private:
    int row_;
    int col_;
    TerrainType terrain_;
    int elevation_{0};
    bool hasIncomingRiver_{false};
    bool hasOutgoingRiver_{false};
    HexDirection incomingRiverDir_{HexDirection::NE};
    HexDirection outgoingRiverDir_{HexDirection::NE};
    int waterLevel_{0};
};

} // namespace game
