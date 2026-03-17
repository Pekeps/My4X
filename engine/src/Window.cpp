#include "engine/Window.h"

#include "raylib.h"

namespace engine::window {

const int DEFAULT_FPS = 60;

void init(int width, int height, const std::string &title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title.c_str());
    SetTargetFPS(DEFAULT_FPS);
}

void shutdown() { CloseWindow(); }

bool isRunning() { return !WindowShouldClose(); }

void beginFrame() {
    BeginDrawing();
    ClearBackground(WHITE);
}

void endFrame() { EndDrawing(); }

} // namespace engine::window
