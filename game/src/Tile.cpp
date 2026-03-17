#include "game/Tile.h"

namespace game {

Tile::Tile(int row, int col, TerrainType terrain) : row_(row), col_(col), terrain_(terrain) {}

int Tile::col() const { return col_; }

int Tile::row() const { return row_; }

TerrainType Tile::terrainType() const { return terrain_; }

} // namespace game
