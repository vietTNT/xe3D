#include "Audio.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "../Managers/Settings.h"

#if defined(VR_ENABLE_AUDIO)
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DECODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#include "miniaudio.h"
#endif

namespace vr {
namespace {

constexpr float kTwoPi = 6.28318530718f;

inline float saw(float phase) { return 2.0f * (phase - std::floor(phase + 0.5f)); }

inline float softClip(float x) {
    if (x > 1.0f) return 1.0f;
    if (x < -1.0f) return -1.0f;
    return x - (x * x * x) / 3.0f * 0.6f;
}

#if defined(VR_ENABLE_AUDIO)
void dataCallback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frameCount) {
    Audio* self = static_cast<Audio*>(device->pUserData);
    if (self) self->renderAudio((float*)output, (unsigned)frameCount, (int)device->playback.channels);
}
#endif

} // namespace

float Audio::noise() {
    m_noise = m_noise * 1664525u + 1013904223u;
    return ((float)((m_noise >> 9) & 0x7FFFFF) / (float)0x400000) - 1.0f;
}

bool Audio::init(const Settings& settings) {
    setVolumes(settings.masterVolume, settings.musicVolume, settings.sfxVolume);
#if defined(VR_ENABLE_AUDIO)
    ma_device_config cfg  = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate        = 48000;
    cfg.dataCallback      = dataCallback;
    cfg.pUserData         = this;

    ma_device* device = new ma_device();
    if (ma_device_init(nullptr, &cfg, device) != MA_SUCCESS) {
        std::fprintf(stderr, "[Audio] no playback device, running silent\n");
        delete device;
        return false;
    }
    m_sampleRate = (float)device->sampleRate;
    if (ma_device_start(device) != MA_SUCCESS) {
        ma_device_uninit(device);
        delete device;
        return false;
    }
    m_device = device;
    m_active = true;
    std::printf("[Audio] procedural synthesis running at %.0f Hz\n", m_sampleRate);
    return true;
#else
    std::printf("[Audio] compiled without audio support (silent mode)\n");
    return false;
#endif
}

void Audio::shutdown() {
#if defined(VR_ENABLE_AUDIO)
    if (m_device) {
        ma_device* device = static_cast<ma_device*>(m_device);
        ma_device_uninit(device);
        delete device;
        m_device = nullptr;
    }
#endif
    m_active = false;
}

void Audio::setVolumes(float master, float music, float sfx) {
    m_master.store(master);
    m_musicVol.store(music);
    m_sfxVol.store(sfx);
}

void Audio::setEngine(float rpm, float throttle, float speedNormalised) {
    m_rpm.store(rpm);
    m_throttle.store(throttle);
    m_speed.store(speedNormalised);
}

void Audio::setTyreSlip(float slip) { m_slip.store(slip); }

void Audio::setMusic(MusicMood mood) { m_mood.store((int)mood); }

void Audio::playBeep(float frequency, float seconds) {
    m_beepFreq.store(frequency);
    m_beepLen.store(seconds);
    m_beepTicket.fetch_add(1);
}

void Audio::playImpact(float strength) {
    m_impactStrength.store(strength);
    m_impactTicket.fetch_add(1);
}

void Audio::playVictory() { m_victoryTicket.fetch_add(1); }

// ------------------------------------------------------------------- the mixer
void Audio::renderAudio(float* out, unsigned frameCount, int channels) {
    if (!out || channels <= 0) return;
    const float dt = 1.0f / m_sampleRate;

    // Pick up one-shot events.
    const int beepTicket = m_beepTicket.load();
    if (beepTicket != m_beepSeen) {
        m_beepSeen   = beepTicket;
        m_beepEnv    = 1.0f;
        m_beepFreqRt = m_beepFreq.load();
    }
    const int impactTicket = m_impactTicket.load();
    if (impactTicket != m_impactSeen) {
        m_impactSeen = impactTicket;
        m_impactEnv  = m_impactStrength.load();
    }
    const int victoryTicket = m_victoryTicket.load();
    if (victoryTicket != m_victorySeen) {
        m_victorySeen = victoryTicket;
        m_victoryEnv  = 1.0f;
        m_victoryTime = 0.0f;
    }

    const float master   = m_master.load();
    const float sfxVol   = m_sfxVol.load() * master;
    const float musicVol = m_musicVol.load() * master;
    const float throttle = m_throttle.load();
    const float slip     = m_slip.load();
    const float speed    = m_speed.load();
    const float targetRpm = m_rpm.load();
    const MusicMood mood  = (MusicMood)m_mood.load();
    const float beepLen  = m_beepLen.load();

    for (unsigned i = 0; i < frameCount; ++i) {
        m_smoothRpm += (targetRpm - m_smoothRpm) * 6.0f * dt;

        // ---- engine: firing frequency plus harmonics ----------------------
        const float f0 = (m_smoothRpm / 60.0f) * 2.0f;   // 4-cylinder-ish firing order
        float engine = 0.0f;
        const float weights[4] = {1.0f, 0.55f, 0.32f, 0.18f};
        for (int h = 0; h < 4; ++h) {
            const float f = f0 * (float)(h + 1);
            m_phase[h] += f * dt;
            if (m_phase[h] > 1.0f) m_phase[h] -= std::floor(m_phase[h]);
            engine += saw(m_phase[h]) * weights[h];
        }
        engine *= 0.16f;
        // Intake growl and exhaust rasp scale with throttle.
        engine += noise() * 0.035f * (0.35f + throttle);
        // One pole low pass so it does not sound harsh.
        const float cutoff = 0.10f + 0.55f * throttle + 0.25f * (m_smoothRpm / 8200.0f);
        m_lpState += (engine - m_lpState) * cutoff;
        float sample = m_lpState * (0.30f + 0.70f * throttle) * (0.55f + 0.45f * speed);

        // ---- wind ----------------------------------------------------------
        sample += noise() * 0.035f * speed * speed;

        // ---- tyre squeal: resonant filtered noise ---------------------------
        if (slip > 0.05f) {
            const float target = noise() * slip;
            m_squealState += (target - m_squealState) * 0.35f;
            m_phase[4] += (1250.0f + 380.0f * slip) * dt;
            if (m_phase[4] > 1.0f) m_phase[4] -= std::floor(m_phase[4]);
            sample += (m_squealState * 0.5f + std::sin(m_phase[4] * kTwoPi) * 0.22f) * slip * 0.42f;
        }

        sample *= sfxVol;

        // ---- impact --------------------------------------------------------
        if (m_impactEnv > 0.0005f) {
            sample += noise() * m_impactEnv * 0.85f * sfxVol;
            m_impactEnv *= 1.0f - 6.0f * dt;
            if (m_impactEnv < 0.0005f) m_impactEnv = 0.0f;
        }

        // ---- countdown beep -------------------------------------------------
        if (m_beepEnv > 0.0005f) {
            m_beepPhase += m_beepFreqRt * dt;
            if (m_beepPhase > 1.0f) m_beepPhase -= std::floor(m_beepPhase);
            sample += std::sin(m_beepPhase * kTwoPi) * m_beepEnv * 0.35f * sfxVol;
            m_beepEnv -= dt / std::max(beepLen, 0.02f);
            if (m_beepEnv < 0.0f) m_beepEnv = 0.0f;
        }

        // ---- victory fanfare -------------------------------------------------
        if (m_victoryEnv > 0.001f) {
            m_victoryTime += dt;
            const float steps[4] = {523.25f, 659.25f, 783.99f, 1046.50f};
            const int   idx      = (int)(m_victoryTime / 0.18f);
            if (idx < 4) {
                m_phase[5] += steps[idx] * dt;
                if (m_phase[5] > 1.0f) m_phase[5] -= std::floor(m_phase[5]);
                sample += std::sin(m_phase[5] * kTwoPi) * 0.30f * sfxVol;
            }
            m_victoryEnv -= dt * 0.45f;
        }

        // ---- music bed --------------------------------------------------------
        if (mood != MusicMood::Silent && musicVol > 0.001f) {
            m_musicTime += dt;
            const float bpm  = (mood == MusicMood::Race) ? 132.0f : 84.0f;
            const float beat = m_musicTime * bpm / 60.0f;
            const int   step = (int)beat % 8;
            // A minor pentatonic arpeggio - simple but never grating.
            const float scale[8] = {220.00f, 261.63f, 329.63f, 392.00f,
                                    329.63f, 261.63f, 196.00f, 174.61f};
            const float env = std::exp(-(beat - std::floor(beat)) * 3.4f);
            const float f   = scale[step] * ((mood == MusicMood::Race) ? 1.0f : 0.5f);
            m_phase[3] += f * dt;
            if (m_phase[3] > 1.0f) m_phase[3] -= std::floor(m_phase[3]);
            float music = saw(m_phase[3]) * env * 0.10f;
            // Soft pad underneath.
            music += std::sin(m_musicTime * 110.0f * kTwoPi * 0.5f) * 0.02f;
            if (mood == MusicMood::Race) {
                const float kick = std::exp(-(beat - std::floor(beat)) * 14.0f);
                music += std::sin(std::floor(beat) * 0.0f + m_musicTime * 55.0f * kTwoPi) * kick *
                         0.18f;
            }
            sample += music * musicVol;
        }

        sample = softClip(sample);
        for (int c = 0; c < channels; ++c) {
            out[(size_t)i * (size_t)channels + (size_t)c] = sample;
        }
    }
}

} // namespace vr
