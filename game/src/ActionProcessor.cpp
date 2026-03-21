#include "game/ActionProcessor.h"

#include "game/Building.h"
#include "game/City.h"
#include "game/GameSession.h"
#include "game/GameState.h"
#include "game/Unit.h"

#include <string>
#include <utility>
#include <variant>

namespace game {

namespace {

/// Convert MoveValidation to a human-readable error string.
std::string moveValidationMessage(MoveValidation val) {
    switch (val) {
    case MoveValidation::Valid:
        return "";
    case MoveValidation::UnitNotFound:
        return "Unit not found";
    case MoveValidation::UnitDead:
        return "Unit is dead";
    case MoveValidation::NoMovementRemaining:
        return "No movement remaining";
    case MoveValidation::NotAdjacent:
        return "Destination is not adjacent";
    case MoveValidation::ImpassableTerrain:
        return "Terrain is impassable";
    case MoveValidation::InsufficientMovement:
        return "Insufficient movement points";
    case MoveValidation::DestinationOccupied:
        return "Destination is occupied";
    case MoveValidation::BlockedByZoneOfControl:
        return "Blocked by zone of control";
    }
    return "Unknown move error";
}

/// Convert AttackValidation to a human-readable error string.
std::string attackValidationMessage(AttackValidation val) {
    switch (val) {
    case AttackValidation::Valid:
        return "";
    case AttackValidation::AttackerNotFound:
        return "Attacker not found";
    case AttackValidation::TargetNotFound:
        return "Target not found";
    case AttackValidation::AttackerDead:
        return "Attacker is dead";
    case AttackValidation::TargetDead:
        return "Target is dead";
    case AttackValidation::SameFaction:
        return "Cannot attack own faction";
    case AttackValidation::NotAtWar:
        return "Not at war with target faction";
    case AttackValidation::OutOfRange:
        return "Target is out of range";
    case AttackValidation::NoMovementRemaining:
        return "No movement remaining for attack";
    }
    return "Unknown attack error";
}

/// Convert CaptureValidation to a human-readable error string.
std::string captureValidationMessage(CaptureValidation val) {
    switch (val) {
    case CaptureValidation::Valid:
        return "";
    case CaptureValidation::UnitNotFound:
        return "Unit not found";
    case CaptureValidation::UnitDead:
        return "Unit is dead";
    case CaptureValidation::NoCityAtPosition:
        return "No city at unit position";
    case CaptureValidation::CityOwnedBySameFaction:
        return "City belongs to same faction";
    case CaptureValidation::NotAtWar:
        return "Not at war with city faction";
    case CaptureValidation::CityIsDefended:
        return "City is defended by enemy units";
    }
    return "Unknown capture error";
}

} // namespace

// ── Public API ─────────────────────────────────────────────────────────────

ProcessedActionResult ActionProcessor::processAction(GameSession &session, PlayerId playerId,
                                                     const ActionVariant &action) {
    // Step 1: Validate player.
    auto playerCheck = validatePlayer(session, playerId);
    if (!playerCheck.success) {
        return playerCheck;
    }

    // Step 2-4: Dispatch to action-specific handler.
    return std::visit(
        [&session, playerId](const auto &act) -> ProcessedActionResult {
            using T = std::decay_t<decltype(act)>;

            if constexpr (std::is_same_v<T, MoveAction>) {
                return processMove(session, playerId, act);
            } else if constexpr (std::is_same_v<T, AttackAction>) {
                return processAttack(session, playerId, act);
            } else if constexpr (std::is_same_v<T, CaptureAction>) {
                return processCapture(session, playerId, act);
            } else if constexpr (std::is_same_v<T, EndTurnAction>) {
                return processEndTurn(session, playerId);
            } else if constexpr (std::is_same_v<T, ProduceUnitAction>) {
                return processProduceUnit(session, playerId, act);
            } else if constexpr (std::is_same_v<T, BuildAction>) {
                return processBuild(session, playerId, act);
            }
        },
        action);
}

// ── Private: action-specific handlers ──────────────────────────────────────

ProcessedActionResult ActionProcessor::processMove(GameSession &session, PlayerId playerId, const MoveAction &action) {
    // Validate and execute via existing action class.
    auto moveResult = action.execute(session.mutableState());

    if (!moveResult.executed) {
        return makeError(moveValidationMessage(moveResult.validation));
    }

    ProcessedActionResult result;
    result.success = true;

    ActionEvent event;
    event.type = ActionEventType::UnitMoved;
    event.playerId = playerId;
    result.events.push_back(std::move(event));

    return result;
}

ProcessedActionResult ActionProcessor::processAttack(GameSession &session, PlayerId playerId,
                                                     const AttackAction &action) {
    // Validate and execute via existing action class.
    auto attackResult = action.execute(session.mutableState());

    if (!attackResult.executed) {
        return makeError(attackValidationMessage(attackResult.validation));
    }

    ProcessedActionResult result;
    result.success = true;

    // Generate attack event.
    ActionEvent attackEvent;
    attackEvent.type = ActionEventType::UnitAttacked;
    attackEvent.playerId = playerId;
    result.events.push_back(std::move(attackEvent));

    // Generate kill events if applicable.
    if (attackResult.combat.defenderDied) {
        ActionEvent killEvent;
        killEvent.type = ActionEventType::UnitKilled;
        killEvent.playerId = playerId;
        killEvent.detail = "Defender killed";
        result.events.push_back(std::move(killEvent));
    }

    if (attackResult.combat.attackerDied) {
        ActionEvent killEvent;
        killEvent.type = ActionEventType::UnitKilled;
        killEvent.playerId = playerId;
        killEvent.detail = "Attacker killed by counter-attack";
        result.events.push_back(std::move(killEvent));
    }

    return result;
}

ProcessedActionResult ActionProcessor::processCapture(GameSession &session, PlayerId playerId,
                                                      const CaptureAction &action) {
    // Validate and execute via existing action class.
    auto captureResult = action.execute(session.mutableState());

    if (!captureResult.executed) {
        return makeError(captureValidationMessage(captureResult.validation));
    }

    ProcessedActionResult result;
    result.success = true;

    // Generate capture event.
    ActionEvent captureEvent;
    captureEvent.type = ActionEventType::CityCaptured;
    captureEvent.playerId = playerId;
    captureEvent.factionId = captureResult.previousOwner;
    captureEvent.detail = captureResult.capturedCityName;
    result.events.push_back(std::move(captureEvent));

    // Generate elimination event if applicable.
    if (captureResult.factionEliminated) {
        ActionEvent eliminationEvent;
        eliminationEvent.type = ActionEventType::FactionEliminated;
        eliminationEvent.playerId = playerId;
        eliminationEvent.factionId = captureResult.eliminatedFactionId;
        result.events.push_back(std::move(eliminationEvent));
    }

    return result;
}

ProcessedActionResult ActionProcessor::processEndTurn(GameSession &session, PlayerId playerId) {
    if (!session.endTurn(playerId)) {
        return makeError("Failed to end turn for player");
    }

    ProcessedActionResult result;
    result.success = true;

    ActionEvent event;
    event.type = ActionEventType::TurnEnded;
    event.playerId = playerId;
    result.events.push_back(std::move(event));

    // Try to advance the turn if all players are ready.
    session.tryAdvanceTurn();

    return result;
}

ProcessedActionResult ActionProcessor::processProduceUnit(GameSession &session, PlayerId playerId,
                                                          const ProduceUnitAction &action) {
    // Verify the city belongs to this player's faction.
    if (!cityBelongsToPlayer(session, playerId, action.cityId)) {
        return makeError("City does not belong to player's faction");
    }

    // Find the city.
    auto *city = session.mutableState().findCity(action.cityId);
    if (city == nullptr) {
        return makeError("City not found");
    }

    // Queue the unit production.
    city->buildQueue().enqueueUnit(action.templateKey, action.productionCost);

    ProcessedActionResult result;
    result.success = true;

    ActionEvent event;
    event.type = ActionEventType::ProductionQueued;
    event.playerId = playerId;
    event.detail = action.templateKey;
    result.events.push_back(std::move(event));

    return result;
}

ProcessedActionResult ActionProcessor::processBuild(GameSession &session, PlayerId playerId,
                                                    const BuildAction &action) {
    // Verify the city belongs to this player's faction.
    if (!cityBelongsToPlayer(session, playerId, action.cityId)) {
        return makeError("City does not belong to player's faction");
    }

    // Find the city.
    auto *city = session.mutableState().findCity(action.cityId);
    if (city == nullptr) {
        return makeError("City not found");
    }

    // Determine building factory from the type string.
    BuildingFactory factory;
    if (action.buildingType == "Farm") {
        factory = makeFarm;
    } else if (action.buildingType == "Mine") {
        factory = makeMine;
    } else if (action.buildingType == "LumberMill") {
        factory = makeLumberMill;
    } else if (action.buildingType == "Barracks") {
        factory = makeBarracks;
    } else if (action.buildingType == "Market") {
        factory = makeMarket;
    } else {
        return makeError("Unknown building type: " + action.buildingType);
    }

    // Enqueue the building construction.
    city->buildQueue().enqueue(std::move(factory), action.targetRow, action.targetCol);

    ProcessedActionResult result;
    result.success = true;

    ActionEvent event;
    event.type = ActionEventType::BuildQueued;
    event.playerId = playerId;
    event.row = action.targetRow;
    event.col = action.targetCol;
    event.detail = action.buildingType;
    result.events.push_back(std::move(event));

    return result;
}

// ── Private: validation helpers ────────────────────────────────────────────

ProcessedActionResult ActionProcessor::validatePlayer(const GameSession &session, PlayerId playerId) {
    const auto *info = session.getPlayerInfo(playerId);
    if (info == nullptr) {
        return makeError("Player not found");
    }

    if (info->status != PlayerStatus::Connected) {
        return makeError("Player is not connected");
    }

    ProcessedActionResult result;
    result.success = true;
    return result;
}

bool ActionProcessor::unitBelongsToPlayer(const GameSession &session, PlayerId playerId, std::size_t unitIndex) {
    FactionId faction = session.getPlayerFaction(playerId);
    if (faction == 0) {
        return false;
    }
    const auto &units = session.getStateSnapshot().units();
    if (unitIndex >= units.size()) {
        return false;
    }
    return units[unitIndex]->factionId() == faction;
}

bool ActionProcessor::cityBelongsToPlayer(const GameSession &session, PlayerId playerId, CityId cityId) {
    FactionId faction = session.getPlayerFaction(playerId);
    if (faction == 0) {
        return false;
    }
    const auto &cities = session.getStateSnapshot().cities();
    for (const auto &city : cities) {
        if (city.id() == cityId) {
            return std::cmp_equal(city.factionId(), faction);
        }
    }
    return false;
}

ProcessedActionResult ActionProcessor::makeError(const std::string &message) {
    ProcessedActionResult result;
    result.success = false;
    result.errorMessage = message;
    return result;
}

} // namespace game
