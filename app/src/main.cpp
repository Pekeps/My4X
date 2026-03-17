#include "engine/BuildingRenderer.h"
#include "engine/Camera.h"
#include "engine/CityRenderer.h"
#include "engine/Input.h"
#include "engine/MapRenderer.h"
#include "engine/UnitRenderer.h"
#include "engine/Window.h"
#include "game/City.h"
#include "game/GameState.h"
#include "game/Warrior.h"

#include "raylib.h"

#include <optional>
#include <string>
#include <vector>

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

const int MAP_ROWS = 30;
const int MAP_COLS = 12;

const int HUD_TEXT_X = 10;
const int HUD_TEXT_Y = 10;
const int HUD_TEXT_SIZE = 20;
const int HUD_HINT_Y = 35;
const int HUD_HINT_SIZE = 16;

const int NO_SELECTION = -1;

// Demo starting resources
const int DEMO_GOLD = 50;
const int DEMO_PRODUCTION = 20;
const int DEMO_FOOD = 30;

// Demo city tile coordinates
const int DEMO_CITY_CENTER_ROW = 5;
const int DEMO_CITY_CENTER_COL = 3;
const int DEMO_CITY_TILE2_ROW = 5;
const int DEMO_CITY_TILE2_COL = 4;
const int DEMO_CITY_TILE3_ROW = 4;
const int DEMO_CITY_TILE3_COL = 3;

// City info panel layout
const int CITY_PANEL_WIDTH = 300;
const int CITY_PANEL_X = SCREEN_WIDTH - CITY_PANEL_WIDTH;
const int CITY_PANEL_Y = 10;
const int CITY_PANEL_TITLE_SIZE = 20;
const int CITY_PANEL_TEXT_SIZE = 16;
const int CITY_PANEL_LINE2_OFFSET = 25;
const int CITY_PANEL_LINE3_OFFSET = 45;

// Resource HUD row offset multiplier
const int RES_HUD_ROW = 2;

static void setupDemoState(game::GameState &state) {
    game::City capital("Ironhold", DEMO_CITY_CENTER_ROW, DEMO_CITY_CENTER_COL);
    capital.addTile(DEMO_CITY_TILE2_ROW, DEMO_CITY_TILE2_COL);
    capital.addTile(DEMO_CITY_TILE3_ROW, DEMO_CITY_TILE3_COL);
    state.addCity(std::move(capital));
    state.addBuilding(game::makeFarm(DEMO_CITY_TILE2_ROW, DEMO_CITY_TILE2_COL));
    state.factionResources() +=
        game::Resource{.gold = DEMO_GOLD, .production = DEMO_PRODUCTION, .food = DEMO_FOOD};
}

static void handleCityClick(const std::vector<game::City> &cities,
                             const engine::hex::TileCoord &tile,
                             std::optional<game::CityId> &selectedCity) {
    bool clickedCity = false;
    for (const auto &city : cities) {
        if (city.containsTile(tile.row, tile.col)) {
            selectedCity = city.id();
            clickedCity = true;
            break;
        }
    }
    if (!clickedCity) {
        selectedCity = std::nullopt;
    }
}

static void drawHud(const game::GameState &state, int selectedUnit,
                    std::optional<game::CityId> selectedCity) {
    std::string turnText = "My4X - Turn " + std::to_string(state.getTurn());
    DrawText(turnText.c_str(), HUD_TEXT_X, HUD_TEXT_Y, HUD_TEXT_SIZE, RAYWHITE);
    DrawText("LMB: select/move | RMB: deselect | SPACE: next turn", HUD_TEXT_X, HUD_HINT_Y,
             HUD_HINT_SIZE, GRAY);

    if (selectedUnit != NO_SELECTION) {
        const auto &unit = state.units().at(selectedUnit);
        std::string unitInfo = unit->name() + " | HP: " + std::to_string(unit->health()) +
                               " | Moves: " + std::to_string(unit->movementRemaining());
        DrawText(unitInfo.c_str(), HUD_TEXT_X, HUD_HINT_Y + HUD_HINT_SIZE, HUD_HINT_SIZE, YELLOW);
    }

    const auto &res = state.factionResources();
    std::string resText = "Gold: " + std::to_string(res.gold) +
                          " | Production: " + std::to_string(res.production) +
                          " | Food: " + std::to_string(res.food);
    DrawText(resText.c_str(), HUD_TEXT_X, HUD_HINT_Y + (HUD_HINT_SIZE * RES_HUD_ROW), HUD_HINT_SIZE,
             GOLD);

    if (selectedCity) {
        for (const auto &city : state.cities()) {
            if (city.id() == *selectedCity) {
                DrawText(city.name().c_str(), CITY_PANEL_X, CITY_PANEL_Y, CITY_PANEL_TITLE_SIZE,
                         WHITE);
                DrawText(("Population: " + std::to_string(city.population())).c_str(), CITY_PANEL_X,
                         CITY_PANEL_Y + CITY_PANEL_LINE2_OFFSET, CITY_PANEL_TEXT_SIZE, LIGHTGRAY);
                DrawText("Build queue: (empty)", CITY_PANEL_X,
                         CITY_PANEL_Y + CITY_PANEL_LINE3_OFFSET, CITY_PANEL_TEXT_SIZE, LIGHTGRAY);
                break;
            }
        }
    }
}

int main() {
    engine::window::init(SCREEN_WIDTH, SCREEN_HEIGHT, "My4X");

    game::GameState state(MAP_ROWS, MAP_COLS);
    engine::Camera camera;

    int selectedUnit = NO_SELECTION;
    std::optional<game::CityId> selectedCity;

    setupDemoState(state);

    while (engine::window::isRunning()) {
        camera.update();

        Camera3D cam = camera.raw();

        // Highlight tile under mouse cursor.
        auto hoveredTile = engine::input::mouseToTile(cam, MAP_ROWS, MAP_COLS);

        engine::window::beginFrame();

        BeginMode3D(cam);
        engine::drawMap(state.map(), hoveredTile);
        engine::drawUnits(state.units(), selectedUnit);
        engine::drawCities(state.cities(), selectedCity);
        engine::drawBuildings(state.buildings());
        EndMode3D();

        // City click detection — takes priority over unit selection.
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoveredTile) {
            handleCityClick(state.cities(), *hoveredTile, selectedCity);
        }

        drawHud(state, selectedUnit, selectedCity);

        if (IsKeyPressed(KEY_SPACE)) {
            state.nextTurn();
            // Reset movement for all units at start of new turn.
            for (auto &unit : state.units()) {
                unit->resetMovement();
            }
        }

        engine::window::endFrame();
    }

    engine::window::shutdown();
    return 0;
}
