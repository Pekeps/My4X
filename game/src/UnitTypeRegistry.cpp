#include "game/UnitTypeRegistry.h"

#include <algorithm>
#include <stdexcept>

namespace game {

namespace {

// ── Warrior stats ────────────────────────────────────────────────────────────
constexpr int WARRIOR_MAX_HEALTH = 100;
constexpr int WARRIOR_ATTACK = 15;
constexpr int WARRIOR_DEFENSE = 10;
constexpr int WARRIOR_MOVEMENT = 2;
constexpr int WARRIOR_ATTACK_RANGE = 1;
constexpr int WARRIOR_PRODUCTION_COST = 20;

// ── Archer stats ─────────────────────────────────────────────────────────────
constexpr int ARCHER_MAX_HEALTH = 70;
constexpr int ARCHER_ATTACK = 12;
constexpr int ARCHER_DEFENSE = 5;
constexpr int ARCHER_MOVEMENT = 2;
constexpr int ARCHER_ATTACK_RANGE = 3;
constexpr int ARCHER_PRODUCTION_COST = 25;

// ── Settler stats ────────────────────────────────────────────────────────────
constexpr int SETTLER_MAX_HEALTH = 50;
constexpr int SETTLER_ATTACK = 0;
constexpr int SETTLER_DEFENSE = 3;
constexpr int SETTLER_MOVEMENT = 3;
constexpr int SETTLER_ATTACK_RANGE = 0;
constexpr int SETTLER_PRODUCTION_COST = 40;

} // namespace

void UnitTypeRegistry::registerTemplate(const std::string &key, UnitTemplate tmpl) {
    if (templates_.contains(key)) {
        throw std::invalid_argument("UnitTemplate already registered: " + key);
    }
    templates_.emplace(key, std::move(tmpl));
}

const UnitTemplate &UnitTypeRegistry::getTemplate(const std::string &key) const {
    auto iter = templates_.find(key);
    if (iter == templates_.end()) {
        throw std::out_of_range("UnitTemplate not found: " + key);
    }
    return iter->second;
}

bool UnitTypeRegistry::hasTemplate(const std::string &key) const { return templates_.contains(key); }

std::vector<std::string> UnitTypeRegistry::allKeys() const {
    std::vector<std::string> keys;
    keys.reserve(templates_.size());
    for (const auto &[key, _] : templates_) {
        keys.push_back(key);
    }
    std::ranges::sort(keys);
    return keys;
}

void UnitTypeRegistry::registerDefaults() {
    registerTemplate("Warrior",
                     UnitTemplate{
                         .name = "Warrior",
                         .maxHealth = WARRIOR_MAX_HEALTH,
                         .attack = WARRIOR_ATTACK,
                         .defense = WARRIOR_DEFENSE,
                         .movementRange = WARRIOR_MOVEMENT,
                         .attackRange = WARRIOR_ATTACK_RANGE,
                         .productionCost = Resource{.gold = 0, .production = WARRIOR_PRODUCTION_COST, .food = 0},
                     });

    registerTemplate("Archer",
                     UnitTemplate{
                         .name = "Archer",
                         .maxHealth = ARCHER_MAX_HEALTH,
                         .attack = ARCHER_ATTACK,
                         .defense = ARCHER_DEFENSE,
                         .movementRange = ARCHER_MOVEMENT,
                         .attackRange = ARCHER_ATTACK_RANGE,
                         .productionCost = Resource{.gold = 0, .production = ARCHER_PRODUCTION_COST, .food = 0},
                     });

    registerTemplate("Settler",
                     UnitTemplate{
                         .name = "Settler",
                         .maxHealth = SETTLER_MAX_HEALTH,
                         .attack = SETTLER_ATTACK,
                         .defense = SETTLER_DEFENSE,
                         .movementRange = SETTLER_MOVEMENT,
                         .attackRange = SETTLER_ATTACK_RANGE,
                         .productionCost = Resource{.gold = 0, .production = SETTLER_PRODUCTION_COST, .food = 0},
                     });
}

} // namespace game
