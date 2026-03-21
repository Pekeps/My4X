// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/AudioManager.h"

#include <gtest/gtest.h>

// ── Registry logic tests (no audio device needed) ───────────────────────────
//
// These tests exercise the key/lookup bookkeeping and volume logic of
// AudioManager without loading any actual audio files, so they run in a
// headless CI environment.

// ── Construction ─────────────────────────────────────────────────────────────

TEST(AudioManagerTest, InitiallyEmpty_NoSounds) {
    engine::AudioManager mgr;
    EXPECT_EQ(mgr.soundCount(), 0);
}

TEST(AudioManagerTest, InitiallyEmpty_NoMusic) {
    engine::AudioManager mgr;
    EXPECT_EQ(mgr.musicCount(), 0);
}

TEST(AudioManagerTest, InitialState_MusicStopped) {
    engine::AudioManager mgr;
    EXPECT_EQ(mgr.musicState(), engine::MusicState::Stopped);
}

TEST(AudioManagerTest, InitialState_CurrentMusicKeyEmpty) {
    engine::AudioManager mgr;
    EXPECT_TRUE(mgr.currentMusicKey().empty());
}

// ── Default volumes ──────────────────────────────────────────────────────────

TEST(AudioManagerTest, DefaultSfxVolume) {
    engine::AudioManager mgr;
    EXPECT_FLOAT_EQ(mgr.sfxVolume(), engine::DEFAULT_SFX_VOLUME);
}

TEST(AudioManagerTest, DefaultMusicVolume) {
    engine::AudioManager mgr;
    EXPECT_FLOAT_EQ(mgr.musicVolume(), engine::DEFAULT_MUSIC_VOLUME);
}

// ── Volume control ───────────────────────────────────────────────────────────

TEST(AudioManagerTest, SetSfxVolume_InRange) {
    engine::AudioManager mgr;
    mgr.setSfxVolume(0.5F);
    EXPECT_FLOAT_EQ(mgr.sfxVolume(), 0.5F);
}

TEST(AudioManagerTest, SetMusicVolume_InRange) {
    engine::AudioManager mgr;
    mgr.setMusicVolume(0.3F);
    EXPECT_FLOAT_EQ(mgr.musicVolume(), 0.3F);
}

TEST(AudioManagerTest, SetSfxVolume_ClampedAboveMax) {
    engine::AudioManager mgr;
    mgr.setSfxVolume(2.0F);
    EXPECT_FLOAT_EQ(mgr.sfxVolume(), engine::MAX_VOLUME);
}

TEST(AudioManagerTest, SetSfxVolume_ClampedBelowMin) {
    engine::AudioManager mgr;
    mgr.setSfxVolume(-1.0F);
    EXPECT_FLOAT_EQ(mgr.sfxVolume(), engine::MIN_VOLUME);
}

TEST(AudioManagerTest, SetMusicVolume_ClampedAboveMax) {
    engine::AudioManager mgr;
    mgr.setMusicVolume(1.5F);
    EXPECT_FLOAT_EQ(mgr.musicVolume(), engine::MAX_VOLUME);
}

TEST(AudioManagerTest, SetMusicVolume_ClampedBelowMin) {
    engine::AudioManager mgr;
    mgr.setMusicVolume(-0.5F);
    EXPECT_FLOAT_EQ(mgr.musicVolume(), engine::MIN_VOLUME);
}

TEST(AudioManagerTest, SetSfxVolume_Zero) {
    engine::AudioManager mgr;
    mgr.setSfxVolume(0.0F);
    EXPECT_FLOAT_EQ(mgr.sfxVolume(), 0.0F);
}

TEST(AudioManagerTest, SetMusicVolume_One) {
    engine::AudioManager mgr;
    mgr.setMusicVolume(1.0F);
    EXPECT_FLOAT_EQ(mgr.musicVolume(), 1.0F);
}

// ── Key lookup (sounds) ─────────────────────────────────────────────────────

TEST(AudioManagerTest, HasSound_ReturnsFalseForMissing) {
    engine::AudioManager mgr;
    EXPECT_FALSE(mgr.hasSound("nonexistent"));
}

TEST(AudioManagerTest, HasSound_ReturnsFalseForSoundKey) {
    engine::AudioManager mgr;
    EXPECT_FALSE(mgr.hasSound(engine::SoundKey::ATTACK));
}

// ── Key lookup (music) ──────────────────────────────────────────────────────

TEST(AudioManagerTest, HasMusic_ReturnsFalseForMissing) {
    engine::AudioManager mgr;
    EXPECT_FALSE(mgr.hasMusic("nonexistent"));
}

TEST(AudioManagerTest, HasMusic_ReturnsFalseForMusicKey) {
    engine::AudioManager mgr;
    EXPECT_FALSE(mgr.hasMusic(engine::MusicKey::MAIN_MENU));
}

// ── Load with missing files ─────────────────────────────────────────────────

TEST(AudioManagerTest, LoadSound_MissingFileReturnsFalse) {
    engine::AudioManager mgr;
    // Loading a nonexistent file should return false and not crash.
    bool result = mgr.loadSound("test", "/nonexistent/path/sound.wav");
    EXPECT_FALSE(result);
    EXPECT_EQ(mgr.soundCount(), 0);
}

TEST(AudioManagerTest, LoadMusic_MissingFileReturnsFalse) {
    engine::AudioManager mgr;
    // Loading a nonexistent file should return false and not crash.
    bool result = mgr.loadMusic("test", "/nonexistent/path/music.ogg");
    EXPECT_FALSE(result);
    EXPECT_EQ(mgr.musicCount(), 0);
}

// ── PlaySound with missing key ──────────────────────────────────────────────

TEST(AudioManagerTest, PlaySound_MissingKeyDoesNotCrash) {
    engine::AudioManager mgr;
    // Should log a warning but not crash.
    mgr.playSound("nonexistent");
}

// ── PlayMusic with missing key ──────────────────────────────────────────────

TEST(AudioManagerTest, PlayMusic_MissingKeyDoesNotCrash) {
    engine::AudioManager mgr;
    // Should log a warning but not crash.
    mgr.playMusic("nonexistent");
    EXPECT_EQ(mgr.musicState(), engine::MusicState::Stopped);
}

// ── StopMusic when not playing ──────────────────────────────────────────────

TEST(AudioManagerTest, StopMusic_WhenStopped_DoesNotCrash) {
    engine::AudioManager mgr;
    mgr.stopMusic();
    EXPECT_EQ(mgr.musicState(), engine::MusicState::Stopped);
}

// ── PauseMusic when not playing ─────────────────────────────────────────────

TEST(AudioManagerTest, PauseMusic_WhenStopped_DoesNotCrash) {
    engine::AudioManager mgr;
    mgr.pauseMusic();
    EXPECT_EQ(mgr.musicState(), engine::MusicState::Stopped);
}

// ── ResumeMusic when not paused ─────────────────────────────────────────────

TEST(AudioManagerTest, ResumeMusic_WhenStopped_DoesNotCrash) {
    engine::AudioManager mgr;
    mgr.resumeMusic();
    EXPECT_EQ(mgr.musicState(), engine::MusicState::Stopped);
}

// ── Update when not playing ─────────────────────────────────────────────────

TEST(AudioManagerTest, Update_WhenStopped_DoesNotCrash) {
    engine::AudioManager mgr;
    mgr.update();
    EXPECT_EQ(mgr.musicState(), engine::MusicState::Stopped);
}

// ── UnloadAll ───────────────────────────────────────────────────────────────

TEST(AudioManagerTest, UnloadAll_OnEmptyManager) {
    engine::AudioManager mgr;
    mgr.unloadAll();
    EXPECT_EQ(mgr.soundCount(), 0);
    EXPECT_EQ(mgr.musicCount(), 0);
}

TEST(AudioManagerTest, UnloadAll_MultipleCalls) {
    engine::AudioManager mgr;
    mgr.unloadAll();
    mgr.unloadAll();
    EXPECT_EQ(mgr.soundCount(), 0);
    EXPECT_EQ(mgr.musicCount(), 0);
}

// ── Move semantics ──────────────────────────────────────────────────────────

TEST(AudioManagerTest, MoveConstruct_TransfersOwnership) {
    engine::AudioManager mgr;
    mgr.setSfxVolume(0.5F);
    mgr.setMusicVolume(0.3F);

    engine::AudioManager moved(std::move(mgr));
    EXPECT_FLOAT_EQ(moved.sfxVolume(), 0.5F);
    EXPECT_FLOAT_EQ(moved.musicVolume(), 0.3F);
    EXPECT_EQ(moved.soundCount(), 0);
    EXPECT_EQ(moved.musicCount(), 0);
}

TEST(AudioManagerTest, MoveAssign_TransfersOwnership) {
    engine::AudioManager mgr;
    mgr.setSfxVolume(0.4F);
    mgr.setMusicVolume(0.2F);

    engine::AudioManager other;
    other = std::move(mgr);
    EXPECT_FLOAT_EQ(other.sfxVolume(), 0.4F);
    EXPECT_FLOAT_EQ(other.musicVolume(), 0.2F);
    EXPECT_EQ(other.soundCount(), 0);
    EXPECT_EQ(other.musicCount(), 0);
}

// ── Sound key constants ─────────────────────────────────────────────────────

TEST(AudioManagerTest, SoundKeys_AreDistinct) {
    // Verify all predefined sound keys are distinct strings.
    std::string keys[] = {
        engine::SoundKey::ATTACK,         engine::SoundKey::HIT,
        engine::SoundKey::DEATH,          engine::SoundKey::BUTTON_CLICK,
        engine::SoundKey::TURN_END,       engine::SoundKey::NOTIFICATION,
        engine::SoundKey::UNIT_MOVE,      engine::SoundKey::CONSTRUCTION_COMPLETE,
    };

    for (std::size_t i = 0; i < engine::SOUND_KEY_COUNT; ++i) {
        for (std::size_t j = i + 1; j < engine::SOUND_KEY_COUNT; ++j) {
            EXPECT_NE(keys[i], keys[j]) << "Duplicate key at indices " << i << " and " << j;
        }
    }
}

// ── Music key constants ─────────────────────────────────────────────────────

TEST(AudioManagerTest, MusicKeys_AreDistinct) {
    // Verify all predefined music keys are distinct strings.
    std::string keys[] = {
        engine::MusicKey::MAIN_MENU,
        engine::MusicKey::GAMEPLAY,
        engine::MusicKey::COMBAT,
    };

    for (std::size_t i = 0; i < engine::MUSIC_KEY_COUNT; ++i) {
        for (std::size_t j = i + 1; j < engine::MUSIC_KEY_COUNT; ++j) {
            EXPECT_NE(keys[i], keys[j]) << "Duplicate key at indices " << i << " and " << j;
        }
    }
}

// ── Volume precision ────────────────────────────────────────────────────────

TEST(AudioManagerTest, VolumeSetGet_PreservesPrecision) {
    engine::AudioManager mgr;
    mgr.setSfxVolume(0.123F);
    EXPECT_FLOAT_EQ(mgr.sfxVolume(), 0.123F);

    mgr.setMusicVolume(0.456F);
    EXPECT_FLOAT_EQ(mgr.musicVolume(), 0.456F);
}

TEST(AudioManagerTest, VolumeSetMultipleTimes_LastWins) {
    engine::AudioManager mgr;
    mgr.setSfxVolume(0.1F);
    mgr.setSfxVolume(0.9F);
    EXPECT_FLOAT_EQ(mgr.sfxVolume(), 0.9F);

    mgr.setMusicVolume(0.2F);
    mgr.setMusicVolume(0.8F);
    EXPECT_FLOAT_EQ(mgr.musicVolume(), 0.8F);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
