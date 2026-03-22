#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

namespace game {

/// Six hex directions for flat-top hexagons in odd-r offset coordinates.
/// Order matters: opposite direction is (dir + 3) % 6.
enum class HexDirection : std::uint8_t {
    NE = 0,
    E = 1,
    SE = 2,
    SW = 3,
    W = 4,
    NW = 5,
};

static constexpr int HEX_DIRECTION_COUNT = 6;

/// Return the opposite direction (180 degrees).
[[nodiscard]] constexpr HexDirection oppositeDirection(HexDirection dir) {
    static constexpr int OPPOSITE_OFFSET = 3;
    return static_cast<HexDirection>((static_cast<int>(dir) + OPPOSITE_OFFSET) % HEX_DIRECTION_COUNT);
}

/// Return the next direction clockwise.
[[nodiscard]] constexpr HexDirection nextDirection(HexDirection dir) {
    return static_cast<HexDirection>((static_cast<int>(dir) + 1) % HEX_DIRECTION_COUNT);
}

/// Return the previous direction counter-clockwise.
[[nodiscard]] constexpr HexDirection previousDirection(HexDirection dir) {
    return static_cast<HexDirection>((static_cast<int>(dir) + HEX_DIRECTION_COUNT - 1) % HEX_DIRECTION_COUNT);
}

/// Return the direction two steps clockwise.
[[nodiscard]] constexpr HexDirection next2Direction(HexDirection dir) {
    return static_cast<HexDirection>((static_cast<int>(dir) + 2) % HEX_DIRECTION_COUNT);
}

/// Return the direction two steps counter-clockwise.
[[nodiscard]] constexpr HexDirection previous2Direction(HexDirection dir) {
    return static_cast<HexDirection>((static_cast<int>(dir) + HEX_DIRECTION_COUNT - 2) % HEX_DIRECTION_COUNT);
}

/// Neighbor coordinate offsets for even rows (row % 2 == 0) in odd-r layout.
/// Order: NE, E, SE, SW, W, NW — matches HexDirection enum.
static constexpr std::array<std::array<int, 2>, HEX_DIRECTION_COUNT> EVEN_ROW_OFFSETS = {{
    {-1, 0},  // NE
    {0, 1},   // E
    {1, 0},   // SE
    {1, -1},  // SW
    {0, -1},  // W
    {-1, -1}, // NW
}};

/// Neighbor coordinate offsets for odd rows (row % 2 == 1) in odd-r layout.
static constexpr std::array<std::array<int, 2>, HEX_DIRECTION_COUNT> ODD_ROW_OFFSETS = {{
    {-1, 1}, // NE
    {0, 1},  // E
    {1, 1},  // SE
    {1, 0},  // SW
    {0, -1}, // W
    {-1, 0}, // NW
}};

/// Compute the neighbor coordinate in a given direction.
/// Returns nullopt if the neighbor is outside [0, mapHeight) x [0, mapWidth).
[[nodiscard]] inline std::optional<std::pair<int, int>> neighborCoord(int row, int col, HexDirection dir, int mapHeight,
                                                                      int mapWidth) {
    const auto &offsets = ((row & 1) == 0) ? EVEN_ROW_OFFSETS : ODD_ROW_OFFSETS;
    int idx = static_cast<int>(dir);
    int nr = row + offsets.at(idx).at(0);
    int nc = col + offsets.at(idx).at(1);
    if (nr < 0 || nr >= mapHeight || nc < 0 || nc >= mapWidth) {
        return std::nullopt;
    }
    return std::pair{nr, nc};
}

/// Iterate over all 6 directions.
static constexpr std::array<HexDirection, HEX_DIRECTION_COUNT> ALL_DIRECTIONS = {
    HexDirection::NE, HexDirection::E, HexDirection::SE, HexDirection::SW, HexDirection::W, HexDirection::NW,
};

} // namespace game
