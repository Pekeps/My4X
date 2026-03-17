#pragma once

#include <string>

namespace engine::window {

void init(int width, int height, const std::string &title);
void shutdown();
[[nodiscard]] bool isRunning();
void beginFrame();
void endFrame();

} // namespace engine::window
