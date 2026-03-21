#include "engine/AudioManager.h"

#include "raylib.h"

#include <algorithm>
#include <utility>

namespace engine {

// ── Destructor ──────────────────────────────────────────────────────────────

AudioManager::~AudioManager() { unloadAll(); }

// ── Move operations ─────────────────────────────────────────────────────────

AudioManager::AudioManager(AudioManager &&other) noexcept
    : sounds_(std::move(other.sounds_)), musicTracks_(std::move(other.musicTracks_)),
      currentMusicKey_(std::move(other.currentMusicKey_)), musicState_(other.musicState_), sfxVolume_(other.sfxVolume_),
      musicVolume_(other.musicVolume_) {
    other.musicState_ = MusicState::Stopped;
    other.sfxVolume_ = DEFAULT_SFX_VOLUME;
    other.musicVolume_ = DEFAULT_MUSIC_VOLUME;
}

AudioManager &AudioManager::operator=(AudioManager &&other) noexcept {
    if (this != &other) {
        unloadAll();
        sounds_ = std::move(other.sounds_);
        musicTracks_ = std::move(other.musicTracks_);
        currentMusicKey_ = std::move(other.currentMusicKey_);
        musicState_ = other.musicState_;
        sfxVolume_ = other.sfxVolume_;
        musicVolume_ = other.musicVolume_;

        other.musicState_ = MusicState::Stopped;
        other.sfxVolume_ = DEFAULT_SFX_VOLUME;
        other.musicVolume_ = DEFAULT_MUSIC_VOLUME;
    }
    return *this;
}

// ── Sound effects ───────────────────────────────────────────────────────────

bool AudioManager::loadSound(const std::string &key, const std::string &filePath) {
    // Already cached — nothing to do.
    if (sounds_.contains(key)) {
        return true;
    }

    // Check if the file exists before asking raylib to load it.
    if (!FileExists(filePath.c_str())) {
        TraceLog( // NOLINT(cppcoreguidelines-pro-type-vararg)
            LOG_WARNING, "AudioManager: sound file not found '%s' for key '%s'", filePath.c_str(), key.c_str());
        return false;
    }

    Sound sound = LoadSound(filePath.c_str());

    // raylib returns a sound with frameCount == 0 on failure.
    if (sound.frameCount == 0) {
        TraceLog( // NOLINT(cppcoreguidelines-pro-type-vararg)
            LOG_WARNING, "AudioManager: failed to load sound '%s' for key '%s'", filePath.c_str(), key.c_str());
        return false;
    }

    sounds_.emplace(key, sound);
    TraceLog(LOG_INFO, // NOLINT(cppcoreguidelines-pro-type-vararg)
             "AudioManager: loaded sound '%s' as '%s'", filePath.c_str(), key.c_str());
    return true;
}

void AudioManager::playSound(const std::string &key) {
    auto iter = sounds_.find(key);
    if (iter == sounds_.end()) {
        TraceLog(LOG_WARNING, // NOLINT(cppcoreguidelines-pro-type-vararg)
                 "AudioManager: sound key '%s' not found, skipping playback", key.c_str());
        return;
    }

    SetSoundVolume(iter->second, sfxVolume_);
    PlaySound(iter->second);
}

bool AudioManager::hasSound(const std::string &key) const { return sounds_.contains(key); }

std::size_t AudioManager::soundCount() const { return sounds_.size(); }

// ── Music ───────────────────────────────────────────────────────────────────

bool AudioManager::loadMusic(const std::string &key, const std::string &filePath) {
    // Already cached — nothing to do.
    if (musicTracks_.contains(key)) {
        return true;
    }

    // Check if the file exists before asking raylib to load it.
    if (!FileExists(filePath.c_str())) {
        TraceLog( // NOLINT(cppcoreguidelines-pro-type-vararg)
            LOG_WARNING, "AudioManager: music file not found '%s' for key '%s'", filePath.c_str(), key.c_str());
        return false;
    }

    Music music = LoadMusicStream(filePath.c_str());

    // raylib returns a music stream with frameCount == 0 on failure.
    if (music.frameCount == 0) {
        TraceLog( // NOLINT(cppcoreguidelines-pro-type-vararg)
            LOG_WARNING, "AudioManager: failed to load music '%s' for key '%s'", filePath.c_str(), key.c_str());
        return false;
    }

    music.looping = true;
    musicTracks_.emplace(key, music);
    TraceLog(LOG_INFO, // NOLINT(cppcoreguidelines-pro-type-vararg)
             "AudioManager: loaded music '%s' as '%s'", filePath.c_str(), key.c_str());
    return true;
}

void AudioManager::playMusic(const std::string &key) {
    auto iter = musicTracks_.find(key);
    if (iter == musicTracks_.end()) {
        TraceLog(LOG_WARNING, // NOLINT(cppcoreguidelines-pro-type-vararg)
                 "AudioManager: music key '%s' not found, skipping playback", key.c_str());
        return;
    }

    // Stop currently playing music if any.
    if (musicState_ != MusicState::Stopped && !currentMusicKey_.empty()) {
        auto currentIter = musicTracks_.find(currentMusicKey_);
        if (currentIter != musicTracks_.end()) {
            StopMusicStream(currentIter->second);
        }
    }

    SetMusicVolume(iter->second, musicVolume_);
    PlayMusicStream(iter->second);
    currentMusicKey_ = key;
    musicState_ = MusicState::Playing;
}

void AudioManager::stopMusic() {
    if (musicState_ == MusicState::Stopped || currentMusicKey_.empty()) {
        return;
    }

    auto iter = musicTracks_.find(currentMusicKey_);
    if (iter != musicTracks_.end()) {
        StopMusicStream(iter->second);
    }

    currentMusicKey_.clear();
    musicState_ = MusicState::Stopped;
}

void AudioManager::pauseMusic() {
    if (musicState_ != MusicState::Playing || currentMusicKey_.empty()) {
        return;
    }

    auto iter = musicTracks_.find(currentMusicKey_);
    if (iter != musicTracks_.end()) {
        PauseMusicStream(iter->second);
    }

    musicState_ = MusicState::Paused;
}

void AudioManager::resumeMusic() {
    if (musicState_ != MusicState::Paused || currentMusicKey_.empty()) {
        return;
    }

    auto iter = musicTracks_.find(currentMusicKey_);
    if (iter != musicTracks_.end()) {
        ResumeMusicStream(iter->second);
    }

    musicState_ = MusicState::Playing;
}

bool AudioManager::hasMusic(const std::string &key) const { return musicTracks_.contains(key); }

std::size_t AudioManager::musicCount() const { return musicTracks_.size(); }

MusicState AudioManager::musicState() const { return musicState_; }

const std::string &AudioManager::currentMusicKey() const { return currentMusicKey_; }

// ── Volume control ──────────────────────────────────────────────────────────

void AudioManager::setSfxVolume(float volume) { sfxVolume_ = clampVolume(volume); }

void AudioManager::setMusicVolume(float volume) {
    musicVolume_ = clampVolume(volume);

    // Apply to currently playing music track immediately.
    if (musicState_ != MusicState::Stopped && !currentMusicKey_.empty()) {
        auto iter = musicTracks_.find(currentMusicKey_);
        if (iter != musicTracks_.end()) {
            SetMusicVolume(iter->second, musicVolume_);
        }
    }
}

float AudioManager::sfxVolume() const { return sfxVolume_; }

float AudioManager::musicVolume() const { return musicVolume_; }

// ── Per-frame update ────────────────────────────────────────────────────────

void AudioManager::update() {
    if (musicState_ != MusicState::Playing || currentMusicKey_.empty()) {
        return;
    }

    auto iter = musicTracks_.find(currentMusicKey_);
    if (iter != musicTracks_.end()) {
        UpdateMusicStream(iter->second);
    }
}

// ── Cleanup ─────────────────────────────────────────────────────────────────

void AudioManager::unloadAll() {
    // Stop any playing music first.
    if (musicState_ != MusicState::Stopped && !currentMusicKey_.empty()) {
        auto iter = musicTracks_.find(currentMusicKey_);
        if (iter != musicTracks_.end()) {
            StopMusicStream(iter->second);
        }
    }

    for (auto &[key, sound] : sounds_) {
        UnloadSound(sound);
        TraceLog(LOG_INFO, // NOLINT(cppcoreguidelines-pro-type-vararg)
                 "AudioManager: unloaded sound '%s'", key.c_str());
    }
    sounds_.clear();

    for (auto &[key, music] : musicTracks_) {
        UnloadMusicStream(music);
        TraceLog(LOG_INFO, // NOLINT(cppcoreguidelines-pro-type-vararg)
                 "AudioManager: unloaded music '%s'", key.c_str());
    }
    musicTracks_.clear();

    currentMusicKey_.clear();
    musicState_ = MusicState::Stopped;
}

// ── Private helpers ─────────────────────────────────────────────────────────

float AudioManager::clampVolume(float volume) { return std::clamp(volume, MIN_VOLUME, MAX_VOLUME); }

} // namespace engine
