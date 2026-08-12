// -----------------------------------------------------------------------------
//  Settings.h - user options, persisted next to the executable as settings.cfg.
// -----------------------------------------------------------------------------
#pragma once

#include <string>

namespace vr {

struct Settings {
    int   windowWidth   = 1600;
    int   windowHeight  = 900;
    bool  fullscreen    = false;
    bool  vsync         = true;
    int   quality       = 2;      ///< 0 low, 1 medium, 2 high
    float masterVolume  = 0.85f;
    float musicVolume   = 0.45f;
    float sfxVolume     = 0.90f;
    int   carColor      = 0;      ///< index into kCarColors
    bool  showFps       = false;
    int   cameraMode    = 0;

    void load();
    void save() const;

    static const char* resolutionLabel(int index);
    static int         resolutionCount();
    static void        resolutionSize(int index, int& outW, int& outH);
    static int         closestResolution(int width, int height);
};

/// The six body colours the player can pick from.
struct CarColorOption {
    const char* name;
    float       r, g, b;
};
extern const CarColorOption kCarColors[6];

} // namespace vr
