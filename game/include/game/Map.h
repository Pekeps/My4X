#pragma once

#include "game/Tile.h"

#include <vector>

namespace game {

class Map {
  public:
    Map(int height, int width);

    [[nodiscard]] const Tile &tile(int row, int col) const;
    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;

  private:
    int width_;
    int height_;
    std::vector<std::vector<Tile>> tiles_;
};

} // namespace game
