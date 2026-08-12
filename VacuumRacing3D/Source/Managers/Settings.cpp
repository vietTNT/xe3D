#include "Settings.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include "../Core/Paths.h"

namespace vr {

const CarColorOption kCarColors[6] = {
    {"RED", 0.72f, 0.045f, 0.05f},  {"BLUE", 0.035f, 0.13f, 0.55f},
    {"BLACK", 0.035f, 0.035f, 0.04f}, {"WHITE", 0.88f, 0.89f, 0.90f},
    {"YELLOW", 0.92f, 0.72f, 0.03f}, {"GREEN", 0.04f, 0.42f, 0.16f},
};

namespace {
struct Res {
    int         w, h;
    const char* label;
};
const Res kResolutions[] = {{1280, 720, "1280 x 720"},  {1366, 768, "1366 x 768"},
                            {1600, 900, "1600 x 900"},  {1920, 1080, "1920 x 1080"},
                            {2560, 1440, "2560 x 1440"}};
constexpr int kResCount = (int)(sizeof(kResolutions) / sizeof(kResolutions[0]));
} // namespace

const char* Settings::resolutionLabel(int index) {
    if (index < 0 || index >= kResCount) index = 0;
    return kResolutions[index].label;
}
int Settings::resolutionCount() { return kResCount; }

void Settings::resolutionSize(int index, int& outW, int& outH) {
    if (index < 0 || index >= kResCount) index = 0;
    outW = kResolutions[index].w;
    outH = kResolutions[index].h;
}

int Settings::closestResolution(int width, int height) {
    int best = 0, bestDiff = 1 << 30;
    for (int i = 0; i < kResCount; ++i) {
        const int diff = std::abs(kResolutions[i].w - width) + std::abs(kResolutions[i].h - height);
        if (diff < bestDiff) {
            bestDiff = diff;
            best     = i;
        }
    }
    return best;
}

void Settings::load() {
    std::ifstream f(paths::writablePath("settings.cfg"));
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);
        std::istringstream iss(val);
        if (key == "width") iss >> windowWidth;
        else if (key == "height") iss >> windowHeight;
        else if (key == "fullscreen") { int v = 0; iss >> v; fullscreen = v != 0; }
        else if (key == "vsync") { int v = 1; iss >> v; vsync = v != 0; }
        else if (key == "quality") iss >> quality;
        else if (key == "master") iss >> masterVolume;
        else if (key == "music") iss >> musicVolume;
        else if (key == "sfx") iss >> sfxVolume;
        else if (key == "color") iss >> carColor;
        else if (key == "showfps") { int v = 0; iss >> v; showFps = v != 0; }
        else if (key == "camera") iss >> cameraMode;
    }
    if (quality < 0 || quality > 2) quality = 2;
    if (carColor < 0 || carColor > 5) carColor = 0;
    std::printf("[Settings] loaded settings.cfg\n");
}

void Settings::save() const {
    std::ofstream f(paths::writablePath("settings.cfg"));
    if (!f) return;
    f << "width=" << windowWidth << "\n"
      << "height=" << windowHeight << "\n"
      << "fullscreen=" << (fullscreen ? 1 : 0) << "\n"
      << "vsync=" << (vsync ? 1 : 0) << "\n"
      << "quality=" << quality << "\n"
      << "master=" << masterVolume << "\n"
      << "music=" << musicVolume << "\n"
      << "sfx=" << sfxVolume << "\n"
      << "color=" << carColor << "\n"
      << "showfps=" << (showFps ? 1 : 0) << "\n"
      << "camera=" << cameraMode << "\n";
}

} // namespace vr
