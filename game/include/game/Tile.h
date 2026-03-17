#pragma once

namespace game {

class Tile {
  public:
    Tile(int row, int col);

    [[nodiscard]] int row() const;
    [[nodiscard]] int col() const;

  private:
    int row_;
    int col_;
};

} // namespace game
