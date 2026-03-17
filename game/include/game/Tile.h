#pragma once

#include "game/TerrainType.h"

namespace game {

class Tile {
  public:
    Tile(int row, int col, TerrainType terrain = TerrainType::Plains);

    [[nodiscard]] int row() const;
    [[nodiscard]] int col() const;
    [[nodiscard]] TerrainType terrainType() const;

  private:
    int row_;
    int col_;
    TerrainType terrain_;
};

} // namespace game
