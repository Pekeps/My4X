#pragma once

#include "raylib.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace engine {

// ── Sound effect key identifiers ────────────────────────────────────────────

/// Predefined sound effect keys used throughout the game.
namespace SoundKey {
// Combat sounds
inline constexpr const char *ATTACK = "combat.attack";
inline constexpr const char *HIT = "combat.hit";
inline constexpr const char *DEATH = "combat.death";

// UI sounds
inline constexpr const char *BUTTON_CLICK = "ui.button_click";
inline constexpr const char *TURN_END = "ui.turn_end";
inline constexpr const char *NOTIFICATION = "ui.notification";

// Movement sounds
inline constexpr const char *UNIT_MOVE = "movement.unit_move";

// Building sounds
inline constexpr const char *CONSTRUCTION_COMPLETE = "building.construction_complete";
} // namespace SoundKey

/// Total number of predefined sound keys.
static constexpr std::size_t SOUND_KEY_COUNT = 8;

// ── Music key identifiers ───────────────────────────────────────────────────

/// Predefined music track keys.
namespace MusicKey {
inline constexpr const char *MAIN_MENU = "music.main_menu";
inline constexpr const char *GAMEPLAY = "music.gameplay";
inline constexpr const char *COMBAT = "music.combat";
} // namespace MusicKey

/// Total number of predefined music keys.
static constexpr std::size_t MUSIC_KEY_COUNT = 3;

// ── Volume constants ────────────────────────────────────────────────────────

/// Default master volume for sound effects.
static constexpr float DEFAULT_SFX_VOLUME = 1.0F;

/// Default master volume for music.
static constexpr float DEFAULT_MUSIC_VOLUME = 0.7F;

/// Minimum allowed volume.
static constexpr float MIN_VOLUME = 0.0F;

/// Maximum allowed volume.
static constexpr float MAX_VOLUME = 1.0F;

// ── Music playback state ────────────────────────────────────────────────────

/// Tracks the current state of music playback.
enum class MusicState : std::uint8_t {
    Stopped,
    Playing,
    Paused,
};

// ── AudioManager ────────────────────────────────────────────────────────────

/// Manages loading, playback, and lifetime of sound effects and music.
///
/// Sound effects are loaded as one-shot audio clips. Music is streamed from
/// disk and requires per-frame update() calls to keep the stream fed.
///
/// IMPORTANT: Loading actual audio files and playback requires an active
/// raylib audio device (InitAudioDevice). The key/lookup logic and volume
/// settings work without one.
class AudioManager {
  public:
    AudioManager() = default;
    ~AudioManager();

    // Non-copyable (audio resources are device-bound).
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    // Movable.
    AudioManager(AudioManager &&other) noexcept;
    AudioManager &operator=(AudioManager &&other) noexcept;

    // ── Sound effects ────────────────────────────────────────────────────

    /// Load a sound effect from `filePath` and store it under `key`.
    /// Returns true on success, false if the file could not be loaded.
    /// If `key` already exists, returns true without reloading.
    bool loadSound(const std::string &key, const std::string &filePath);

    /// Play a previously loaded sound effect by key.
    /// If the key is not found, logs a warning and does nothing.
    void playSound(const std::string &key);

    /// Check whether a sound effect is registered under the given key.
    [[nodiscard]] bool hasSound(const std::string &key) const;

    /// Return the number of currently loaded sound effects.
    [[nodiscard]] std::size_t soundCount() const;

    // ── Music ────────────────────────────────────────────────────────────

    /// Load a music stream from `filePath` and store it under `key`.
    /// Returns true on success, false if the file could not be loaded.
    /// If `key` already exists, returns true without reloading.
    bool loadMusic(const std::string &key, const std::string &filePath);

    /// Start playing a music track by key with looping enabled.
    /// Stops any currently playing music first.
    /// If the key is not found, logs a warning and does nothing.
    void playMusic(const std::string &key);

    /// Stop the currently playing music.
    void stopMusic();

    /// Pause the currently playing music.
    void pauseMusic();

    /// Resume paused music.
    void resumeMusic();

    /// Check whether a music track is registered under the given key.
    [[nodiscard]] bool hasMusic(const std::string &key) const;

    /// Return the number of currently loaded music tracks.
    [[nodiscard]] std::size_t musicCount() const;

    /// Return the current music playback state.
    [[nodiscard]] MusicState musicState() const;

    /// Return the key of the currently active music track, or empty string.
    [[nodiscard]] const std::string &currentMusicKey() const;

    // ── Volume control ───────────────────────────────────────────────────

    /// Set the master volume for sound effects (clamped to [0.0, 1.0]).
    void setSfxVolume(float volume);

    /// Set the master volume for music (clamped to [0.0, 1.0]).
    void setMusicVolume(float volume);

    /// Get the current sound effects volume.
    [[nodiscard]] float sfxVolume() const;

    /// Get the current music volume.
    [[nodiscard]] float musicVolume() const;

    // ── Per-frame update ─────────────────────────────────────────────────

    /// Must be called each frame to update music streaming.
    void update();

    // ── Cleanup ──────────────────────────────────────────────────────────

    /// Unload all sounds and music, reset state.
    void unloadAll();

  private:
    /// Clamp a volume value to the valid range.
    [[nodiscard]] static float clampVolume(float volume);

    /// Loaded sound effects indexed by key.
    std::unordered_map<std::string, Sound> sounds_;

    /// Loaded music streams indexed by key.
    std::unordered_map<std::string, Music> musicTracks_;

    /// Key of the currently playing/paused music track.
    std::string currentMusicKey_;

    /// Current music playback state.
    MusicState musicState_ = MusicState::Stopped;

    /// Master volume for sound effects.
    float sfxVolume_ = DEFAULT_SFX_VOLUME;

    /// Master volume for music.
    float musicVolume_ = DEFAULT_MUSIC_VOLUME;
};

} // namespace engine
