#include "game/Tile.h"

namespace game {

Tile::Tile(int row, int col, TerrainType terrain) : row_(row), col_(col), terrain_(terrain) {}

int Tile::col() const { return col_; }

int Tile::row() const { return row_; }

TerrainType Tile::terrainType() const { return terrain_; }

void Tile::setTerrainType(TerrainType terrain) { terrain_ = terrain; }

int Tile::elevation() const { return elevation_; }

void Tile::setElevation(int elevation) { elevation_ = elevation; }

bool Tile::hasIncomingRiver() const { return hasIncomingRiver_; }
bool Tile::hasOutgoingRiver() const { return hasOutgoingRiver_; }
bool Tile::hasRiver() const { return hasIncomingRiver_ || hasOutgoingRiver_; }
bool Tile::hasRiverBeginOrEnd() const { return hasIncomingRiver_ != hasOutgoingRiver_; }

bool Tile::hasRiverThroughEdge(HexDirection dir) const {
    return (hasIncomingRiver_ && incomingRiverDir_ == dir) || (hasOutgoingRiver_ && outgoingRiverDir_ == dir);
}

HexDirection Tile::incomingRiverDirection() const { return incomingRiverDir_; }
HexDirection Tile::outgoingRiverDirection() const { return outgoingRiverDir_; }

void Tile::setOutgoingRiver(HexDirection dir) {
    hasOutgoingRiver_ = true;
    outgoingRiverDir_ = dir;
}

void Tile::removeOutgoingRiver() { hasOutgoingRiver_ = false; }

void Tile::setIncomingRiver(HexDirection dir) {
    hasIncomingRiver_ = true;
    incomingRiverDir_ = dir;
}

void Tile::removeIncomingRiver() { hasIncomingRiver_ = false; }

int Tile::waterLevel() const { return waterLevel_; }
void Tile::setWaterLevel(int level) { waterLevel_ = level; }
bool Tile::isUnderwater() const { return waterLevel_ > elevation_; }

} // namespace game
