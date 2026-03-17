#include "engine/Camera.h"
#include "engine/Input.h"
#include "engine/MapRenderer.h"
#include "engine/UnitRenderer.h"
#include "engine/Window.h"
#include "game/GameState.h"
#include "game/Map.h"
#include "game/Warrior.h"

#include "raylib.h"

#include <memory>
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

int main() {
    engine::window::init(SCREEN_WIDTH, SCREEN_HEIGHT, "My4X");

    game::GameState state;
    game::Map map(MAP_ROWS, MAP_COLS);
    engine::Camera camera;

    std::vector<std::unique_ptr<game::Unit>> units;
    units.push_back(std::make_unique<game::Warrior>(5, 3));

    int selectedUnit = NO_SELECTION;

    while (engine::window::isRunning()) {
        camera.update();

        Camera3D cam = camera.raw();

        // Handle mouse click: select unit or move selected unit.
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto clicked = engine::input::mouseToTile(cam, MAP_ROWS, MAP_COLS);
            if (clicked) {
                // Check if a unit is on the clicked tile.
                int clickedUnit = NO_SELECTION;
                for (int i = 0; i < static_cast<int>(units.size()); ++i) {
                    if (units.at(i)->isAlive() && units.at(i)->row() == clicked->row &&
                        units.at(i)->col() == clicked->col) {
                        clickedUnit = i;
                        break;
                    }
                }

                if (clickedUnit != NO_SELECTION) {
                    // Clicked on a unit: select it.
                    selectedUnit = clickedUnit;
                } else if (selectedUnit != NO_SELECTION && units.at(selectedUnit)->movementRemaining() > 0) {
                    // Clicked on empty tile with a unit selected: move it.
                    units.at(selectedUnit)->moveTo(clicked->row, clicked->col);
                }
            }
        }

        // Right-click to deselect.
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            selectedUnit = NO_SELECTION;
        }

        // Highlight tile under mouse cursor.
        auto hoveredTile = engine::input::mouseToTile(cam, MAP_ROWS, MAP_COLS);

        engine::window::beginFrame();

        BeginMode3D(cam);
        engine::drawMap(map, hoveredTile);
        engine::drawUnits(units, selectedUnit);
        EndMode3D();

        // HUD
        std::string turnText = "My4X - Turn " + std::to_string(state.getTurn());
        DrawText(turnText.c_str(), HUD_TEXT_X, HUD_TEXT_Y, HUD_TEXT_SIZE, RAYWHITE);
        DrawText("LMB: select/move | RMB: deselect | SPACE: next turn", HUD_TEXT_X, HUD_HINT_Y, HUD_HINT_SIZE, GRAY);

        if (selectedUnit != NO_SELECTION) {
            const auto &unit = units.at(selectedUnit);
            std::string unitInfo =
                unit->name() + " | HP: " + std::to_string(unit->health()) +
                " | Moves: " + std::to_string(unit->movementRemaining());
            DrawText(unitInfo.c_str(), HUD_TEXT_X, HUD_HINT_Y + HUD_HINT_SIZE + 5, HUD_HINT_SIZE, YELLOW);
        }

        if (IsKeyPressed(KEY_SPACE)) {
            state.nextTurn();
            // Reset movement for all units at start of new turn.
            for (auto &unit : units) {
                unit->resetMovement();
            }
        }

        engine::window::endFrame();
    }

    engine::window::shutdown();
    return 0;
}
