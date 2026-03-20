#pragma once

namespace game {

/// Coordinate pair identifying a tile on the hex grid.
struct TileCoord {
    int row;
    int col;

    [[nodiscard]] friend bool operator==(const TileCoord &lhs, const TileCoord &rhs) = default;
    [[nodiscard]] friend auto operator<=>(const TileCoord &lhs, const TileCoord &rhs) = default;
};

} // namespace game
