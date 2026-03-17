#pragma once

#include <cstdint>
#include <string>

namespace game {

using BuildingId = std::uint32_t;

struct Building {
    BuildingId id = 0;
    int row = 0;
    int col = 0;
    std::string name;
};

} // namespace game
