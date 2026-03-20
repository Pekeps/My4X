#include "game/DiplomacyManager.h"

#include "game/FactionRegistry.h"

#include <algorithm>

namespace game {

DiplomacyManager::FactionPair DiplomacyManager::makeKey(FactionId a, FactionId b) {
    return a < b ? FactionPair{a, b} : FactionPair{b, a};
}

DiplomacyState DiplomacyManager::defaultRelation(FactionType typeA, FactionType typeB) {
    // Ensure a canonical ordering so we only need to check each combination once.
    // Order: Player < NeutralHostile < NeutralPassive (by underlying value).
    auto ta = typeA;
    auto tb = typeB;
    if (static_cast<std::uint8_t>(ta) > static_cast<std::uint8_t>(tb)) {
        std::swap(ta, tb);
    }

    // Same type pairs
    if (ta == tb) {
        if (ta == FactionType::Player) {
            return DiplomacyState::War; // free-for-all
        }
        return DiplomacyState::Peace; // neutrals of the same kind are peaceful
    }

    // Player vs NeutralHostile
    if (ta == FactionType::Player && tb == FactionType::NeutralHostile) {
        return DiplomacyState::War;
    }

    // Player vs NeutralPassive
    if (ta == FactionType::Player && tb == FactionType::NeutralPassive) {
        return DiplomacyState::Peace;
    }

    // NeutralHostile vs NeutralPassive
    if (ta == FactionType::NeutralHostile && tb == FactionType::NeutralPassive) {
        return DiplomacyState::War;
    }

    // Fallback (should not be reached with current enum values)
    return DiplomacyState::Peace;
}

DiplomacyState DiplomacyManager::getRelation(FactionId factionA, FactionId factionB) const {
    if (factionA == factionB) {
        return DiplomacyState::Peace;
    }

    auto key = makeKey(factionA, factionB);
    auto iter = relations_.find(key);
    if (iter != relations_.end()) {
        return iter->second;
    }

    return DiplomacyState::Peace;
}

void DiplomacyManager::setRelation(FactionId factionA, FactionId factionB, DiplomacyState state) {
    if (factionA == factionB) {
        return;
    }

    auto key = makeKey(factionA, factionB);
    relations_[key] = state;
}

bool DiplomacyManager::areAtWar(FactionId factionA, FactionId factionB) const {
    return getRelation(factionA, factionB) == DiplomacyState::War;
}

bool DiplomacyManager::areAllied(FactionId factionA, FactionId factionB) const {
    return getRelation(factionA, factionB) == DiplomacyState::Alliance;
}

void DiplomacyManager::initializeDefaults(const FactionRegistry &registry) {
    const auto &factions = registry.allFactions();
    for (std::size_t i = 0; i < factions.size(); ++i) {
        for (std::size_t j = i + 1; j < factions.size(); ++j) {
            auto state = defaultRelation(factions[i].type(), factions[j].type());
            setRelation(factions[i].id(), factions[j].id(), state);
        }
    }
}

std::size_t DiplomacyManager::size() const { return relations_.size(); }

void DiplomacyManager::clear() { relations_.clear(); }

} // namespace game
