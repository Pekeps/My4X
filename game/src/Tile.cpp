#include "game/Tile.h"

namespace game {

Tile::Tile(int row, int col) : col_(col), row_(row) {}

int Tile::col() const { return col_; }

int Tile::row() const { return row_; }

} // namespace game
