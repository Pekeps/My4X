#include "game/CaptureAction.h"

#include "game/City.h"
#include "game/GameState.h"
#include "game/TileRegistry.h"
#include "game/Unit.h"

#include <utility>

namespace game {

CaptureAction::CaptureAction(std::size_t unitIndex) : unitIndex_(unitIndex) {}

CaptureValidation CaptureAction::validate(const GameState &state) const {
    const auto &units = state.units();

    if (unitIndex_ >= units.size()) {
        return CaptureValidation::UnitNotFound;
    }

    const auto &unit = *units[unitIndex_];

    if (!unit.isAlive()) {
        return CaptureValidation::UnitDead;
    }

    // Find a city whose center is at the unit's position.
    // Use a const_cast-free approach: scan cities directly.
    const auto &cities = state.cities();
    const City *targetCity = nullptr;
    for (const auto &city : cities) {
        if (city.centerRow() == unit.row() && city.centerCol() == unit.col()) {
            targetCity = &city;
            break;
        }
    }

    if (targetCity == nullptr) {
        return CaptureValidation::NoCityAtPosition;
    }

    // Cannot capture own city.
    if (static_cast<FactionId>(targetCity->factionId()) == unit.factionId()) {
        return CaptureValidation::CityOwnedBySameFaction;
    }

    // Must be at war with the city's faction.
    auto cityFactionId = static_cast<FactionId>(targetCity->factionId());
    if (!state.diplomacy().areAtWar(unit.factionId(), cityFactionId)) {
        return CaptureValidation::NotAtWar;
    }

    // City must be undefended (no enemy units on any city tile).
    if (isCityDefended(state, *targetCity)) {
        return CaptureValidation::CityIsDefended;
    }

    return CaptureValidation::Valid;
}

CaptureResult CaptureAction::execute(GameState &state) const {
    CaptureResult result;
    result.validation = validate(state);

    if (result.validation != CaptureValidation::Valid) {
        result.executed = false;
        return result;
    }

    const auto &unit = *state.units()[unitIndex_];
    City *city = findCityCenterAt(state, unit.row(), unit.col());

    // Store previous owner before transfer.
    result.previousOwner = static_cast<FactionId>(city->factionId());
    result.newOwner = unit.factionId();
    result.capturedCityName = city->name();

    // Transfer ownership.
    city->setFactionId(static_cast<int>(unit.factionId()));

    // Clear production queue (new owner sets their own priorities).
    city->buildQueue().clear();

    // Buildings are preserved (no action needed).

    result.executed = true;

    // Check if the previous owner is eliminated.
    if (isFactionEliminated(state, result.previousOwner)) {
        result.factionEliminated = true;
        result.eliminatedFactionId = result.previousOwner;
    }

    return result;
}

bool CaptureAction::isFactionEliminated(const GameState &state, FactionId factionId) {
    // Check for any remaining cities.
    for (const auto &city : state.cities()) {
        if (std::cmp_equal(city.factionId(), factionId)) {
            return false;
        }
    }

    // Check for any remaining alive units.
    for (const auto &unit : state.units()) {
        if (unit->isAlive() && unit->factionId() == factionId) {
            return false;
        }
    }

    return true;
}

City *CaptureAction::findCityCenterAt(GameState &state, int row, int col) {
    for (auto &city : state.mutableCities()) {
        if (city.centerRow() == row && city.centerCol() == col) {
            return &city;
        }
    }
    return nullptr;
}

bool CaptureAction::isCityDefended(const GameState &state, const City &city) {
    auto cityFactionId = static_cast<FactionId>(city.factionId());

    for (const auto &[tileRow, tileCol] : city.tiles()) {
        auto unitsOnTile = state.registry().unitsAt(tileRow, tileCol);
        for (const auto *unitPtr : unitsOnTile) {
            if (unitPtr->isAlive() && unitPtr->factionId() == cityFactionId) {
                return true;
            }
        }
    }
    return false;
}

std::size_t CaptureAction::unitIndex() const { return unitIndex_; }

} // namespace game
