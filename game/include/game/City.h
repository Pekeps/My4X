#pragma once

#include <cstdint>
#include <string>

namespace game {

using CityId = std::uint32_t;

struct City {
    CityId id = 0;
    int row = 0;
    int col = 0;
    std::string name;
};

} // namespace game
