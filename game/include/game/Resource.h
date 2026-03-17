#pragma once

namespace game {

struct Resource {
    int gold = 0;
    int production = 0;
    int food = 0;

    [[nodiscard]] friend Resource operator+(Resource lhs, const Resource &rhs) {
        lhs += rhs;
        return lhs;
    }

    [[nodiscard]] friend Resource operator-(Resource lhs, const Resource &rhs) {
        lhs -= rhs;
        return lhs;
    }

    Resource &operator+=(const Resource &rhs) {
        gold += rhs.gold;
        production += rhs.production;
        food += rhs.food;
        return *this;
    }

    Resource &operator-=(const Resource &rhs) {
        gold -= rhs.gold;
        production -= rhs.production;
        food -= rhs.food;
        return *this;
    }

    [[nodiscard]] friend bool operator>=(const Resource &lhs, const Resource &rhs) {
        return lhs.gold >= rhs.gold && lhs.production >= rhs.production && lhs.food >= rhs.food;
    }

    [[nodiscard]] friend bool operator==(const Resource &lhs, const Resource &rhs) = default;
};

} // namespace game
