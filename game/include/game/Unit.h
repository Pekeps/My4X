#pragma once

#include <string>

namespace game {

class Unit {
  public:
    Unit(int row, int col, int health, int movement, std::string name);
    virtual ~Unit() = default;

    Unit(const Unit &) = default;
    Unit &operator=(const Unit &) = default;
    Unit(Unit &&) = default;
    Unit &operator=(Unit &&) = default;

    [[nodiscard]] int row() const;
    [[nodiscard]] int col() const;
    void moveTo(int row, int col);

    [[nodiscard]] int health() const;
    [[nodiscard]] int maxHealth() const;
    [[nodiscard]] bool isAlive() const;
    void takeDamage(int amount);

    [[nodiscard]] int movement() const;
    [[nodiscard]] int movementRemaining() const;
    void resetMovement();

    [[nodiscard]] const std::string &name() const;

  private:
    int row_;
    int col_;
    int health_;
    int maxHealth_;
    int movement_;
    int movementRemaining_;
    std::string name_;
};

} // namespace game
