#pragma once

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

  private:
    int row_;
    int col_;
    TerrainType terrain_;
    int elevation_{0};
};

} // namespace game
