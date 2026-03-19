#include "game/Building.h"
#include "game/City.h"
#include "game/GameState.h"
#include "game/SaveLoad.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace {

/// RAII helper that creates a temporary directory and removes it on destruction.
class TempDir {
  public:
    TempDir() {
        path_ = std::filesystem::temp_directory_path() / ("my4x_test_" + std::to_string(counter_++));
        std::filesystem::create_directories(path_);
    }
    ~TempDir() { std::filesystem::remove_all(path_); }

    TempDir(const TempDir &) = delete;
    TempDir &operator=(const TempDir &) = delete;

    [[nodiscard]] std::string path() const { return path_.string(); }
    [[nodiscard]] std::string file(const std::string &name) const { return (path_ / name).string(); }

  private:
    std::filesystem::path path_;
    static inline int counter_ = 0;
};

game::UnitTypeRegistry makeTestRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

} // namespace

TEST(SaveLoadTest, RoundTripEmptyState) {
    TempDir dir;
    std::string filepath = dir.file("test.bin");

    game::GameState state(5, 5, 42);
    ASSERT_TRUE(game::saveGame(state, filepath));

    game::GameState loaded = game::loadGame(filepath);
    EXPECT_EQ(loaded.getTurn(), 1);
    EXPECT_EQ(loaded.map().height(), 5);
    EXPECT_EQ(loaded.map().width(), 5);
    EXPECT_EQ(loaded.cities().size(), 0U);
    EXPECT_EQ(loaded.buildings().size(), 0U);
    EXPECT_EQ(loaded.units().size(), 0U);
}

TEST(SaveLoadTest, RoundTripFullState) {
    TempDir dir;
    std::string filepath = dir.file("test.bin");
    auto reg = makeTestRegistry();

    game::GameState state(10, 10, 42);

    for (int i = 0; i < 5; ++i) {
        state.nextTurn();
    }

    game::City city("Rome", 3, 4, 1);
    city.growPopulation(3);
    city.addTile(3, 5);
    state.addCity(std::move(city));

    state.addBuilding(game::makeFarm(5, 5));
    state.addBuilding(game::makeMine(2, 2));

    auto warrior = std::make_unique<game::Warrior>(7, 7, reg);
    warrior->takeDamage(20);
    state.addUnit(std::move(warrior));

    state.factionResources().gold = 100;
    state.factionResources().production = 50;
    state.factionResources().food = 25;

    ASSERT_TRUE(game::saveGame(state, filepath));
    game::GameState loaded = game::loadGame(filepath);

    EXPECT_EQ(loaded.getTurn(), 6);
    EXPECT_EQ(loaded.map().height(), 10);
    EXPECT_EQ(loaded.map().width(), 10);

    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 10; ++c) {
            EXPECT_EQ(loaded.map().tile(r, c).terrainType(), state.map().tile(r, c).terrainType());
        }
    }

    ASSERT_EQ(loaded.cities().size(), 1U);
    EXPECT_EQ(loaded.cities()[0].name(), "Rome");
    EXPECT_EQ(loaded.cities()[0].population(), 4);
    EXPECT_EQ(loaded.cities()[0].centerRow(), 3);
    EXPECT_EQ(loaded.cities()[0].centerCol(), 4);

    ASSERT_EQ(loaded.buildings().size(), 2U);
    EXPECT_EQ(loaded.buildings()[0].name(), "Farm");
    EXPECT_EQ(loaded.buildings()[1].name(), "Mine");

    ASSERT_EQ(loaded.units().size(), 1U);
    EXPECT_EQ(loaded.units()[0]->name(), "Warrior");
    EXPECT_EQ(loaded.units()[0]->health(), 80);

    EXPECT_EQ(loaded.factionResources().gold, 100);
    EXPECT_EQ(loaded.factionResources().production, 50);
    EXPECT_EQ(loaded.factionResources().food, 25);
}

TEST(SaveLoadTest, CreatesParentDirectories) {
    TempDir dir;
    std::string filepath = dir.file("sub/dir/test.bin");

    game::GameState state(5, 5, 42);
    ASSERT_TRUE(game::saveGame(state, filepath));
    EXPECT_TRUE(std::filesystem::exists(filepath));

    game::GameState loaded = game::loadGame(filepath);
    EXPECT_EQ(loaded.getTurn(), 1);
}

TEST(SaveLoadTest, LoadMissingFileThrows) {
    EXPECT_THROW((void)game::loadGame("/nonexistent/path/test.bin"), std::runtime_error);
}

TEST(SaveLoadTest, LoadCorruptFileThrows) {
    TempDir dir;
    std::string filepath = dir.file("corrupt.bin");

    std::ofstream out(filepath, std::ios::binary);
    out << "this is not valid protobuf data";
    out.close();

    EXPECT_THROW((void)game::loadGame(filepath), std::runtime_error);
}

TEST(SaveLoadTest, LoadEmptyFileThrows) {
    TempDir dir;
    std::string filepath = dir.file("empty.bin");

    std::ofstream out(filepath, std::ios::binary);
    out.close();

    EXPECT_THROW((void)game::loadGame(filepath), std::runtime_error);
}

TEST(SaveLoadTest, GenerateSavePathFormat) {
    std::string path = game::generateSavePath();

    EXPECT_EQ(path.substr(0, 6), "saves/");
    EXPECT_EQ(path.substr(path.size() - 4), ".bin");

    EXPECT_NE(path.find("savegame_"), std::string::npos);
}

TEST(SaveLoadTest, LatestSavePathThrowsWhenNoSaves) {
    TempDir dir;
    auto original = std::filesystem::current_path();
    std::filesystem::current_path(dir.path());

    EXPECT_THROW((void)game::latestSavePath(), std::runtime_error);

    std::filesystem::current_path(original);
}

TEST(SaveLoadTest, LatestSavePathFindsNewestFile) {
    TempDir dir;
    auto original = std::filesystem::current_path();
    std::filesystem::current_path(dir.path());

    std::filesystem::create_directories("saves");

    game::GameState state1(5, 5, 42);
    ASSERT_TRUE(game::saveGame(state1, "saves/savegame_20260101_000000.bin"));

    auto older = std::filesystem::file_time_type::clock::now() - std::chrono::seconds(10);
    std::filesystem::last_write_time("saves/savegame_20260101_000000.bin", older);

    game::GameState state2(5, 5, 42);
    state2.nextTurn();
    ASSERT_TRUE(game::saveGame(state2, "saves/savegame_20260101_000001.bin"));

    std::string latest = game::latestSavePath();
    std::filesystem::path expected("saves/savegame_20260101_000001.bin");
    EXPECT_EQ(std::filesystem::path(latest), expected);

    game::GameState loaded = game::loadGame(latest);
    EXPECT_EQ(loaded.getTurn(), 2);

    std::filesystem::current_path(original);
}
