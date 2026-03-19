#include "game/Serialization.h"

#include "game/BuildQueue.h"
#include "game/Building.h"
#include "game/City.h"
#include "game/Resource.h"
#include "game/TerrainType.h"
#include "game/Warrior.h"

#include "game_state.pb.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace game {

namespace {

game_proto::Resource toProto(const Resource &r) {
    game_proto::Resource proto;
    proto.set_gold(r.gold);
    proto.set_production(r.production);
    proto.set_food(r.food);
    return proto;
}

Resource fromProto(const game_proto::Resource &proto) {
    Resource r;
    r.gold = proto.gold();
    r.production = proto.production();
    r.food = proto.food();
    return r;
}

game_proto::TerrainType toProtoTerrain(TerrainType t) {
    switch (t) { // NOLINT(bugprone-branch-clone)
    case TerrainType::Plains:
        return game_proto::PLAINS;
    case TerrainType::Hills:
        return game_proto::HILLS;
    case TerrainType::Forest:
        return game_proto::FOREST;
    case TerrainType::Water:
        return game_proto::WATER;
    case TerrainType::Mountain:
        return game_proto::MOUNTAIN;
    }
    throw std::invalid_argument("Unknown TerrainType: " + std::to_string(static_cast<int>(t)));
}

TerrainType fromProtoTerrain(game_proto::TerrainType t) {
    switch (t) {
    case game_proto::PLAINS:
        return TerrainType::Plains;
    case game_proto::HILLS:
        return TerrainType::Hills;
    case game_proto::FOREST:
        return TerrainType::Forest;
    case game_proto::WATER:
        return TerrainType::Water;
    case game_proto::MOUNTAIN:
        return TerrainType::Mountain;
    default:
        throw std::invalid_argument("Unknown proto TerrainType: " + std::to_string(static_cast<int>(t)));
    }
}

game_proto::UnitType toProtoUnitType(const Unit &unit) {
    if (unit.name() == "Warrior") {
        return game_proto::WARRIOR;
    }
    throw std::invalid_argument("Unknown unit type: " + std::string(unit.name()));
}

std::unique_ptr<Unit> fromProtoUnitType(game_proto::UnitType type, int row, int col) {
    switch (type) {
    case game_proto::WARRIOR:
        return std::make_unique<Warrior>(row, col);
    case game_proto::UNIT_TYPE_UNSPECIFIED:
        throw std::invalid_argument("Cannot deserialize UNIT_TYPE_UNSPECIFIED");
    default:
        throw std::invalid_argument("Unknown proto UnitType: " + std::to_string(static_cast<int>(type)));
    }
}

BuildingFactory buildingFactoryByName(const std::string &name) {
    if (name == "Farm") {
        return makeFarm;
    }
    if (name == "Mine") {
        return makeMine;
    }
    if (name == "Market") {
        return makeMarket;
    }
    throw std::invalid_argument("Unknown building type for build queue: " + name);
}

constexpr uint32_t SCHEMA_VERSION = 1;

} // namespace

std::string serializeGameState(const GameState &state) {
    game_proto::GameState proto;
    proto.set_schema_version(SCHEMA_VERSION);
    proto.set_turn(state.getTurn());
    proto.set_next_city_id(state.nextCityId());
    proto.set_next_building_id(state.nextBuildingId());

    // Map
    const Map &m = state.map();
    game_proto::Map *protoMap = proto.mutable_map();
    protoMap->set_width(m.width());
    protoMap->set_height(m.height());
    for (int row = 0; row < m.height(); ++row) {
        for (int col = 0; col < m.width(); ++col) {
            const Tile &t = m.tile(row, col);
            game_proto::Tile *protoTile = protoMap->add_tiles();
            protoTile->set_row(t.row());
            protoTile->set_col(t.col());
            protoTile->set_terrain(toProtoTerrain(t.terrainType()));
        }
    }

    // Cities
    for (const City &city : state.cities()) {
        game_proto::City *protoCity = proto.add_cities();
        protoCity->set_id(city.id());
        protoCity->set_name(city.name());
        protoCity->set_center_row(city.centerRow());
        protoCity->set_center_col(city.centerCol());
        protoCity->set_faction_id(city.factionId());
        protoCity->set_population(city.population());
        for (const auto &[tRow, tCol] : city.tiles()) {
            protoCity->add_tile_rows(tRow);
            protoCity->add_tile_cols(tCol);
        }

        protoCity->set_food_surplus(city.foodSurplus());

        // Build queue
        const BuildQueue &bq = city.buildQueue();
        protoCity->set_accumulated_production(bq.accumulatedProduction());
        for (const BuildQueueItem &item : bq.allItems()) {
            game_proto::BuildQueueItem *protoItem = protoCity->add_build_queue_items();
            protoItem->set_name(item.name);
            protoItem->set_production_cost(item.productionCost);
            protoItem->set_target_row(item.targetRow);
            protoItem->set_target_col(item.targetCol);
        }
    }

    // Buildings
    for (const Building &building : state.buildings()) {
        game_proto::Building *protoBuilding = proto.add_buildings();
        protoBuilding->set_id(building.id());
        protoBuilding->set_name(building.name());
        for (const TileCoord &coord : building.occupiedTiles()) {
            protoBuilding->add_tile_rows(coord.row);
            protoBuilding->add_tile_cols(coord.col);
        }
        *protoBuilding->mutable_yield_per_turn() = toProto(building.yieldPerTurn());
        *protoBuilding->mutable_construction_cost() = toProto(building.constructionCost());
        for (TerrainType terrain : building.allowedTerrains()) {
            protoBuilding->add_allowed_terrains(toProtoTerrain(terrain));
        }
    }

    // Units
    for (const auto &unit : state.units()) {
        game_proto::Unit *protoUnit = proto.add_units();
        protoUnit->set_type(toProtoUnitType(*unit));
        protoUnit->set_row(unit->row());
        protoUnit->set_col(unit->col());
        protoUnit->set_health(unit->health());
        protoUnit->set_movement_remaining(unit->movementRemaining());
        protoUnit->set_faction_id(0);
    }

    // Faction resources
    *proto.mutable_faction_resources() = toProto(state.factionResources());

    std::string data;
    if (!proto.SerializeToString(&data)) {
        throw std::runtime_error("Failed to serialize GameState");
    }
    return data;
}

GameState deserializeGameState(const std::string &data) {
    if (data.empty()) {
        throw std::runtime_error("Cannot deserialize empty data");
    }

    game_proto::GameState proto;
    if (!proto.ParseFromString(data)) {
        throw std::runtime_error("Failed to deserialize GameState");
    }

    if (proto.map().height() <= 0 || proto.map().width() <= 0) {
        throw std::runtime_error("Invalid map dimensions in serialized data");
    }

    GameState state(proto.map().height(), proto.map().width());

    // Turn
    state.setTurn(proto.turn());

    // Restore map terrain from proto tiles
    Map &map = state.mutableMap();
    for (const game_proto::Tile &protoTile : proto.map().tiles()) {
        map.tile(protoTile.row(), protoTile.col()).setTerrainType(fromProtoTerrain(protoTile.terrain()));
    }

    // Cities (restore with original IDs)
    for (const game_proto::City &protoCity : proto.cities()) {
        City city(protoCity.name(), protoCity.center_row(), protoCity.center_col(), protoCity.faction_id());
        city.setId(protoCity.id());
        city.growPopulation(protoCity.population() - 1);
        for (int i = 0; i < protoCity.tile_rows_size(); ++i) {
            city.addTile(protoCity.tile_rows(i), protoCity.tile_cols(i));
        }

        // Restore build queue
        BuildQueue &bq = city.buildQueue();
        for (const game_proto::BuildQueueItem &protoItem : protoCity.build_queue_items()) {
            BuildQueueItem item;
            item.name = protoItem.name();
            item.productionCost = protoItem.production_cost();
            item.targetRow = protoItem.target_row();
            item.targetCol = protoItem.target_col();
            item.factory = buildingFactoryByName(protoItem.name());
            bq.restoreItem(std::move(item));
        }
        bq.setAccumulatedProduction(protoCity.accumulated_production());

        city.setFoodSurplus(protoCity.food_surplus());

        state.restoreCity(std::move(city));
    }
    state.setNextCityId(proto.next_city_id());

    // Buildings (restore with original IDs and allowed terrains)
    for (const game_proto::Building &protoBuilding : proto.buildings()) {
        std::set<TileCoord> tiles;
        for (int i = 0; i < protoBuilding.tile_rows_size(); ++i) {
            tiles.insert(TileCoord{.row = protoBuilding.tile_rows(i), .col = protoBuilding.tile_cols(i)});
        }
        std::set<TerrainType> allowedTerrains;
        for (int i = 0; i < protoBuilding.allowed_terrains_size(); ++i) {
            allowedTerrains.insert(fromProtoTerrain(protoBuilding.allowed_terrains(i)));
        }
        Resource yieldPerTurn = fromProto(protoBuilding.yield_per_turn());
        Resource constructionCost = fromProto(protoBuilding.construction_cost());
        Building building(protoBuilding.name(), yieldPerTurn, constructionCost, std::move(tiles),
                          std::move(allowedTerrains));
        building.setId(protoBuilding.id());
        state.restoreBuilding(std::move(building));
    }
    state.setNextBuildingId(proto.next_building_id());

    // Units (restore health and movement)
    for (const game_proto::Unit &protoUnit : proto.units()) {
        auto unit = fromProtoUnitType(protoUnit.type(), protoUnit.row(), protoUnit.col());
        unit->setHealth(protoUnit.health());
        unit->setMovementRemaining(protoUnit.movement_remaining());
        state.addUnit(std::move(unit));
    }

    // Faction resources
    state.factionResources() = fromProto(proto.faction_resources());

    return state;
}

} // namespace game
