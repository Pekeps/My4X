#include "engine/Camera.h"
#include "engine/MapRenderer.h"
#include "engine/Window.h"
#include "game/GameState.h"
#include "game/Map.h"

#include "raylib.h"

#include <string>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int MAP_ROWS = 30;
const int MAP_COLS = 12;

const int HUD_TEXT_X = 10;
const int HUD_TEXT_Y = 10;
const int HUD_TEXT_SIZE = 20;
const int HUD_HINT_Y = 35;
const int HUD_HINT_SIZE = 16;

int main() {
    engine::window::init(SCREEN_WIDTH, SCREEN_HEIGHT, "My4X");

    game::GameState state;
    game::Map map(MAP_ROWS, MAP_COLS);
    engine::Camera camera;

    while (engine::window::isRunning()) {
        camera.update();

        engine::window::beginFrame();

        Camera3D cam = camera.raw();
        BeginMode3D(cam);
        engine::drawMap(map);
        EndMode3D();

        // HUD drawn in 2D on top of the 3D scene.
        std::string turnText = "My4X - Turn " + std::to_string(state.getTurn());
        DrawText(turnText.c_str(), HUD_TEXT_X, HUD_TEXT_Y, HUD_TEXT_SIZE, RAYWHITE);
        DrawText("WASD: pan | Q/E: rotate | Scroll: zoom | SPACE: next turn",
                 HUD_TEXT_X, HUD_HINT_Y, HUD_HINT_SIZE, GRAY);

        if (IsKeyPressed(KEY_SPACE)) {
            state.nextTurn();
        }

        engine::window::endFrame();
    }

    engine::window::shutdown();
    return 0;
}
