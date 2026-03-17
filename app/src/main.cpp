#include "engine/Window.h"
#include "game/GameState.h"

#include "raylib.h"

#include <string>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int TEXT_X = 320;
const int TEXT_Y = 280;
const int TEXT_SIZE = 20;

const int HINT_X = 280;
const int HINT_Y = 320;
const int HINT_SIZE = 16;

int main() {
    engine::window::init(SCREEN_WIDTH, SCREEN_HEIGHT, "My4X");

    game::GameState state;

    while (engine::window::isRunning()) {
        engine::window::beginFrame();

        std::string turnText = "My4X - Turn " + std::to_string(state.getTurn());
        DrawText(turnText.c_str(), TEXT_X, TEXT_Y, TEXT_SIZE, RAYWHITE);
        DrawText("Press SPACE for next turn", HINT_X, HINT_Y, HINT_SIZE, GRAY);

        if (IsKeyPressed(KEY_SPACE)) {
            state.nextTurn();
        }

        engine::window::endFrame();
    }

    engine::window::shutdown();
    return 0;
}
