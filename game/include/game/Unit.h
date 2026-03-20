#pragma once

#include "game/Faction.h"
#include "game/UnitTemplate.h"

#include <array>
#include <string>

namespace game {

// ── Experience and leveling constants ────────────────────────────────────────

/// Maximum level a unit can achieve.
static constexpr int MAX_UNIT_LEVEL = 5;

/// Number of XP thresholds (one per level above 0).
static constexpr int XP_THRESHOLD_COUNT = MAX_UNIT_LEVEL;

/// XP required to reach each level (cumulative thresholds).
/// Level 1 = 100 XP, Level 2 = 250, Level 3 = 500, Level 4 = 750, Level 5 = 1000.
static constexpr std::array<int, XP_THRESHOLD_COUNT> XP_THRESHOLDS = {100, 250, 500, 750, 1000};

/// Flat bonus to attack and defense granted per level.
static constexpr int STAT_BONUS_PER_LEVEL = 2;

/// XP awarded per point of damage dealt when attacking.
static constexpr int XP_PER_DAMAGE_DEALT = 3;

/// Bonus XP awarded for landing the killing blow.
static constexpr int XP_KILL_BONUS = 50;

/// XP awarded to a defender for surviving an attack.
static constexpr int XP_DEFEND_SURVIVE = 10;

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

    /// Move this unit to the given position, deducting the specified movement
    /// cost from its remaining movement points.
    void moveTo(int row, int col, int cost);

    [[nodiscard]] int health() const;
    [[nodiscard]] int maxHealth() const;
    [[nodiscard]] bool isAlive() const;
    void takeDamage(int amount);

    /// Base attack from template (without level bonuses).
    [[nodiscard]] int baseAttack() const;
    /// Base defense from template (without level bonuses).
    [[nodiscard]] int baseDefense() const;

    /// Effective attack including level bonuses.
    [[nodiscard]] int attack() const;
    /// Effective defense including level bonuses.
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

    // ── Experience / leveling ────────────────────────────────────────────────

    /// Current accumulated experience points.
    [[nodiscard]] int experience() const;

    /// Current level (0 = base, up to MAX_UNIT_LEVEL).
    [[nodiscard]] int level() const;

    /// Add experience points. Automatically advances level when thresholds
    /// are crossed. Returns true if the unit leveled up during this call.
    bool addExperience(int xp);

    /// Directly set experience (e.g. for deserialization). Recomputes level.
    void setExperience(int xp);

    /// Returns true if this unit leveled up since the last call to
    /// clearLevelUpFlag(). Used for visual indicators.
    [[nodiscard]] bool justLeveledUp() const;

    /// Clear the level-up indicator flag (call after displaying the indicator).
    void clearLevelUpFlag();

  private:
    /// Recompute level_ from experience_ based on XP_THRESHOLDS.
    void recomputeLevel();

    int row_;
    int col_;
    int health_;
    int movementRemaining_;
    FactionId factionId_;
    UnitTemplate template_;
    int experience_ = 0;
    int level_ = 0;
    bool justLeveledUp_ = false;
};

} // namespace game
