#include "game/FactionRegistry.h"

#include <algorithm>
#include <utility>

namespace game {

FactionId FactionRegistry::addFaction(std::string name, FactionType type, std::uint8_t colorIndex) {
    FactionId id = nextFactionId_++;
    factions_.emplace_back(id, std::move(name), type, colorIndex);
    return id;
}

const Faction &FactionRegistry::getFaction(FactionId id) const {
    auto iter = std::ranges::find_if(factions_, [id](const Faction &f) { return f.id() == id; });
    if (iter == factions_.end()) {
        throw std::out_of_range("Faction not found");
    }
    return *iter;
}

const Faction *FactionRegistry::findFaction(FactionId id) const {
    auto iter = std::ranges::find_if(factions_, [id](const Faction &f) { return f.id() == id; });
    if (iter == factions_.end()) {
        return nullptr;
    }
    return &(*iter);
}

Faction &FactionRegistry::getMutableFaction(FactionId id) {
    auto iter = std::ranges::find_if(factions_, [id](Faction &f) { return f.id() == id; });
    if (iter == factions_.end()) {
        throw std::out_of_range("Faction not found");
    }
    return *iter;
}

const std::vector<Faction> &FactionRegistry::allFactions() const { return factions_; }

std::vector<Faction> &FactionRegistry::allMutableFactions() { return factions_; }

std::vector<const Faction *> FactionRegistry::playerFactions() const {
    std::vector<const Faction *> result;
    for (const auto &faction : factions_) {
        if (faction.type() == FactionType::Player) {
            result.push_back(&faction);
        }
    }
    return result;
}

std::vector<const Faction *> FactionRegistry::neutralFactions() const {
    std::vector<const Faction *> result;
    for (const auto &faction : factions_) {
        if (faction.type() == FactionType::NeutralHostile || faction.type() == FactionType::NeutralPassive) {
            result.push_back(&faction);
        }
    }
    return result;
}

FactionId FactionRegistry::nextFactionId() const { return nextFactionId_; }

void FactionRegistry::setNextFactionId(FactionId id) { nextFactionId_ = id; }

void FactionRegistry::restoreFaction(Faction faction) { factions_.push_back(std::move(faction)); }

} // namespace game
