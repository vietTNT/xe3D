// -----------------------------------------------------------------------------
//  Audio.h - fully procedural sound.
//
//  There are no audio files in this project: the engine note, tyre squeal, wind,
//  impacts, countdown beeps and the music bed are synthesised in the audio
//  callback from the car's live telemetry. Build with -DVR_ENABLE_AUDIO=OFF to
//  compile a silent stub (the game plays exactly the same).
// -----------------------------------------------------------------------------
#pragma once

#include <atomic>

namespace vr {

struct Settings;

enum class MusicMood { Silent, Menu, Race };

class Audio {
public:
    bool init(const Settings& settings);
    void shutdown();
    bool active() const { return m_active; }

    void setVolumes(float master, float music, float sfx);

    /// Continuous parameters, pushed every frame from the player's car.
    void setEngine(float rpm, float throttle, float speedNormalised);
    void setTyreSlip(float slip);
    void setMusic(MusicMood mood);

    /// One-shot events.
    void playBeep(float frequency, float seconds);
    void playImpact(float strength);
    void playVictory();

    /// Audio thread entry point (public so the C callback can reach it).
    void renderAudio(float* output, unsigned frameCount, int channels);

private:
    // ---- shared parameters (written by the game thread, read by the mixer)
    std::atomic<float> m_rpm{900.0f};
    std::atomic<float> m_throttle{0.0f};
    std::atomic<float> m_speed{0.0f};
    std::atomic<float> m_slip{0.0f};
    std::atomic<float> m_master{0.85f};
    std::atomic<float> m_musicVol{0.45f};
    std::atomic<float> m_sfxVol{0.9f};
    std::atomic<int>   m_mood{(int)MusicMood::Silent};

    std::atomic<int>   m_beepTicket{0};
    std::atomic<float> m_beepFreq{880.0f};
    std::atomic<float> m_beepLen{0.12f};
    std::atomic<int>   m_impactTicket{0};
    std::atomic<float> m_impactStrength{0.0f};
    std::atomic<int>   m_victoryTicket{0};

    // ---- mixer state (audio thread only)
    int   m_beepSeen    = 0;
    int   m_impactSeen  = 0;
    int   m_victorySeen = 0;
    float m_phase[6]    = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float m_beepEnv     = 0.0f;
    float m_beepPhase   = 0.0f;
    float m_beepFreqRt  = 880.0f;
    float m_impactEnv   = 0.0f;
    float m_victoryEnv  = 0.0f;
    float m_victoryTime = 0.0f;
    float m_musicTime   = 0.0f;
    float m_lpState     = 0.0f;
    float m_squealState = 0.0f;
    float m_smoothRpm   = 900.0f;
    unsigned m_noise    = 22222u;
    float m_sampleRate  = 48000.0f;

    bool  m_active = false;
    void* m_device = nullptr;

    float noise();
};

} // namespace vr
