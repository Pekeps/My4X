#pragma once

#include "game/Faction.h"
#include "game/UnitTemplate.h"

#include <string>

namespace game {

class Unit {
  public:
    /// Construct a unit at (row, col) whose stats come from the given template,
    /// owned by the faction with the given ID.
    Unit(int row, int col, const UnitTemplate &tmpl, FactionId factionId);
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

    [[nodiscard]] int attack() const;
    [[nodiscard]] int defense() const;
    [[nodiscard]] int attackRange() const;

    [[nodiscard]] int movement() const;
    [[nodiscard]] int movementRemaining() const;
    void resetMovement();
    void setHealth(int health);
    void setMovementRemaining(int remaining);

    [[nodiscard]] const std::string &name() const;

    /// Return the faction that owns this unit.
    [[nodiscard]] FactionId factionId() const;

    /// Return the template that defines this unit's type.
    [[nodiscard]] const UnitTemplate &unitTemplate() const;

  private:
    int row_;
    int col_;
    int health_;
    int movementRemaining_;
    FactionId factionId_;
    UnitTemplate template_;
};

} // namespace game
