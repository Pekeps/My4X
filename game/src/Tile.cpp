#include "game/Tile.h"

namespace game {

Tile::Tile(int row, int col, TerrainType terrain) : row_(row), col_(col), terrain_(terrain) {}

int Tile::col() const { return col_; }

int Tile::row() const { return row_; }

TerrainType Tile::terrainType() const { return terrain_; }

void Tile::setTerrainType(TerrainType terrain) { terrain_ = terrain; }

int Tile::elevation() const { return elevation_; }

void Tile::setElevation(int elevation) { elevation_ = elevation; }

} // namespace game
