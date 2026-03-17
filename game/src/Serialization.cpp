#include "game/Serialization.h"

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
    switch (t) {
    case TerrainType::Hills:
        return game_proto::HILLS;
    case TerrainType::Forest:
        return game_proto::FOREST;
    case TerrainType::Water:
        return game_proto::WATER;
    case TerrainType::Plains:
    case TerrainType::Mountain:
    default:
        return game_proto::PLAINS;
    }
}

TerrainType fromProtoTerrain(game_proto::TerrainType t) {
    switch (t) {
    case game_proto::HILLS:
        return TerrainType::Hills;
    case game_proto::FOREST:
        return TerrainType::Forest;
    case game_proto::WATER:
        return TerrainType::Water;
    case game_proto::PLAINS:
    default:
        return TerrainType::Plains;
    }
}

} // namespace

std::string serializeGameState(const GameState &state) {
    game_proto::GameState proto;
    proto.set_turn(state.getTurn());

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
    }

    // Units
    for (const auto &unit : state.units()) {
        game_proto::Unit *protoUnit = proto.add_units();
        protoUnit->set_type(game_proto::WARRIOR);
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
    game_proto::GameState proto;
    if (!proto.ParseFromString(data)) {
        throw std::runtime_error("Failed to deserialize GameState");
    }

    GameState state(proto.map().height(), proto.map().width());

    // Advance turn counter (starts at 1, so call nextTurn turn-1 times)
    for (int i = 1; i < proto.turn(); ++i) {
        state.nextTurn();
    }

    // Cities
    for (const game_proto::City &protoCity : proto.cities()) {
        City city(protoCity.name(), protoCity.center_row(), protoCity.center_col(), protoCity.faction_id());
        city.growPopulation(protoCity.population() - 1);
        for (int i = 0; i < protoCity.tile_rows_size(); ++i) {
            city.addTile(protoCity.tile_rows(i), protoCity.tile_cols(i));
        }
        state.addCity(std::move(city));
    }

    // Buildings
    for (const game_proto::Building &protoBuilding : proto.buildings()) {
        std::set<TileCoord> tiles;
        for (int i = 0; i < protoBuilding.tile_rows_size(); ++i) {
            tiles.insert(TileCoord{.row = protoBuilding.tile_rows(i), .col = protoBuilding.tile_cols(i)});
        }
        Resource yieldPerTurn = fromProto(protoBuilding.yield_per_turn());
        Resource constructionCost = fromProto(protoBuilding.construction_cost());
        Building building(protoBuilding.name(), yieldPerTurn, constructionCost, std::move(tiles));
        state.addBuilding(std::move(building));
    }

    // Units
    for (const game_proto::Unit &protoUnit : proto.units()) {
        state.addUnit(std::make_unique<Warrior>(protoUnit.row(), protoUnit.col()));
    }

    // Faction resources
    state.factionResources() = fromProto(proto.faction_resources());

    return state;
}

} // namespace game
