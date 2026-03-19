#include "game/Faction.h"

#include <utility>

namespace game {

Faction::Faction(FactionId id, std::string name, FactionType type, std::uint8_t colorIndex)
    : id_(id), name_(std::move(name)), type_(type), colorIndex_(colorIndex) {}

FactionId Faction::id() const { return id_; }
const std::string &Faction::name() const { return name_; }
FactionType Faction::type() const { return type_; }
std::uint8_t Faction::colorIndex() const { return colorIndex_; }

void Faction::setName(std::string name) { name_ = std::move(name); }
void Faction::setColorIndex(std::uint8_t colorIndex) { colorIndex_ = colorIndex; }

const Resource &Faction::stockpile() const { return stockpile_; }
Resource &Faction::stockpile() { return stockpile_; }

void Faction::addResources(const Resource &amount) { stockpile_ += amount; }

bool Faction::spendResources(const Resource &cost) {
    if (stockpile_ >= cost) {
        stockpile_ -= cost;
        return true;
    }
    return false;
}

} // namespace game
