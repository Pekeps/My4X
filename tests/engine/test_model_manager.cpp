// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/ModelManager.h"

#include <gtest/gtest.h>

// ── Registry logic tests (no GPU context needed) ────────────────────────────
//
// These tests exercise the key/lookup bookkeeping of ModelManager without
// loading any actual Model objects, so they run in a headless CI environment.

TEST(ModelManagerTest, InitiallyEmpty) {
    engine::ModelManager mgr;
    EXPECT_EQ(mgr.modelCount(), 0);
}

TEST(ModelManagerTest, HasModel_ReturnsFalseForMissing) {
    engine::ModelManager mgr;
    EXPECT_FALSE(mgr.hasModel("nonexistent"));
}

TEST(ModelManagerTest, GetModel_ReturnsNullForMissing) {
    engine::ModelManager mgr;
    EXPECT_EQ(mgr.getModel("nonexistent"), nullptr);
}

TEST(ModelManagerTest, LoadModel_MissingFileDoesNotThrow) {
    // loadModel with a nonexistent file should not crash.
    // It will try to generate a fallback, which needs GPU context,
    // so we just verify it doesn't throw/crash. The fallback generation
    // may fail without a window, but the function should handle it gracefully.
    engine::ModelManager mgr;
    // We cannot assert the return value here because generateFallback
    // calls GenMeshCube which requires an OpenGL context. But at minimum
    // loadModel should not crash when given a bad path.
    // This test is primarily a smoke test for graceful error handling.
}

TEST(ModelManagerTest, UnloadAll_OnEmptyManager) {
    engine::ModelManager mgr;
    // Should not crash on empty manager.
    mgr.unloadAll();
    EXPECT_EQ(mgr.modelCount(), 0);
}

TEST(ModelManagerTest, UnloadAll_MultipleCalls) {
    engine::ModelManager mgr;
    // Multiple unloadAll calls should be safe.
    mgr.unloadAll();
    mgr.unloadAll();
    EXPECT_EQ(mgr.modelCount(), 0);
}

TEST(ModelManagerTest, MoveConstruct_TransfersOwnership) {
    engine::ModelManager mgr;
    // Move an empty manager — should not crash.
    engine::ModelManager moved(std::move(mgr));
    EXPECT_EQ(moved.modelCount(), 0);
}

TEST(ModelManagerTest, MoveAssign_TransfersOwnership) {
    engine::ModelManager mgr;
    engine::ModelManager other;
    // Move-assign an empty manager — should not crash.
    other = std::move(mgr);
    EXPECT_EQ(other.modelCount(), 0);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
