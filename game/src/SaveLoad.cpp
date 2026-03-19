#include "game/SaveLoad.h"

#include "game/Serialization.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace game {

bool saveGame(const GameState &state, const std::string &filepath) {
    try {
        std::string data = serializeGameState(state);

        std::filesystem::path path(filepath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream out(filepath, std::ios::binary | std::ios::trunc);
        if (!out) {
            return false;
        }

        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        return out.good();
    } catch (...) {
        return false;
    }
}

GameState loadGame(const std::string &filepath) {
    if (!std::filesystem::exists(filepath)) {
        throw std::runtime_error("Save file not found: " + filepath);
    }

    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open save file: " + filepath);
    }

    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (data.empty()) {
        throw std::runtime_error("Save file is empty: " + filepath);
    }

    return deserializeGameState(data);
}

std::string generateSavePath() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    std::ostringstream oss;
    oss << "saves/savegame_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".bin";
    return oss.str();
}

std::string latestSavePath() {
    const std::filesystem::path savesDir("saves");

    if (!std::filesystem::exists(savesDir) || !std::filesystem::is_directory(savesDir)) {
        throw std::runtime_error("No saves directory found");
    }

    std::filesystem::path latest;
    std::filesystem::file_time_type latestTime{};
    bool found = false;

    for (const auto &entry : std::filesystem::directory_iterator(savesDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".bin") {
            auto writeTime = entry.last_write_time();
            if (!found || writeTime > latestTime) {
                latest = entry.path();
                latestTime = writeTime;
                found = true;
            }
        }
    }

    if (!found) {
        throw std::runtime_error("No save files found in saves/");
    }

    return latest.string();
}

} // namespace game
