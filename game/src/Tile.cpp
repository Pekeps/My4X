#include "game/Tile.h"

namespace game {

Tile::Tile(int row, int col, TerrainType terrain) : col_(col), row_(row), terrain_(terrain) {}

int Tile::col() const { return col_; }

int Tile::row() const { return row_; }

TerrainType Tile::terrainType() const { return terrain_; }

} // namespace game
