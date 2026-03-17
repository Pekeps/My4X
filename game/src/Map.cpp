#include "game/Map.h"

#include <cassert>

namespace game {

Map::Map(int height, int width) : width_(width), height_(height) {
    assert(width > 0);
    assert(height > 0);
    tiles_.reserve(height);
    for (int row = 0; row < height; ++row) {
        std::vector<Tile> rowTiles;
        rowTiles.reserve(width);
        for (int col = 0; col < width; ++col) {
            rowTiles.emplace_back(row, col);
        }
        tiles_.push_back(std::move(rowTiles));
    }
}

const Tile &Map::tile(int row, int col) const {
    assert(row >= 0 && row < height_);
    assert(col >= 0 && col < width_);
    return tiles_[row][col];
}

int Map::width() const { return width_; }

int Map::height() const { return height_; }

} // namespace game
