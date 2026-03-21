#pragma once

#include <cstdint>

namespace engine {

/// Classification of the edge between two hex cells based on elevation difference.
enum class HexEdgeType : std::uint8_t {
    Flat,  ///< Same elevation
    Slope, ///< Exactly 1 elevation level difference
    Cliff, ///< 2+ elevation levels difference
};

/// Classify the edge type between two elevations.
[[nodiscard]] constexpr HexEdgeType classifyEdge(int elevationA, int elevationB) {
    int diff = (elevationA > elevationB) ? (elevationA - elevationB) : (elevationB - elevationA);
    if (diff == 0) {
        return HexEdgeType::Flat;
    }
    if (diff == 1) {
        return HexEdgeType::Slope;
    }
    return HexEdgeType::Cliff;
}

} // namespace engine
