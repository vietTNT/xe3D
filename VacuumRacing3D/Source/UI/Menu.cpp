#include "Menu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../Core/Input.h"
#include "../Core/Window.h"
#include "../Managers/RaceManager.h"
#include "../Managers/Settings.h"
#include "../Utilities/MathUtils.h"
#include "../Vehicle/Vehicle.h"
#include "UIRenderer.h"

namespace vr {
namespace {

const glm::vec3 kInk(0.95f, 0.96f, 0.98f);
const glm::vec3 kDim(0.62f, 0.64f, 0.68f);
const glm::vec3 kAccent(0.98f, 0.34f, 0.10f);
const glm::vec3 kPanel(0.035f, 0.04f, 0.05f);

std::string formatTime(float seconds) {
    char buf[24];
    math::formatLapTime(seconds, buf, sizeof(buf));
    return std::string(buf);
}

} // namespace

void MenuSystem::beginScreen(int itemCount, Input& input) {
    m_count = std::max(1, itemCount);
    if (input.keyPressed(GLFW_KEY_DOWN) || input.keyPressed(GLFW_KEY_S)) {
        m_focus = (m_focus + 1) % m_count;
    }
    if (input.keyPressed(GLFW_KEY_UP) || input.keyPressed(GLFW_KEY_W)) {
        m_focus = (m_focus - 1 + m_count) % m_count;
    }
    m_focus = math::clampf((float)m_focus, 0.0f, (float)(m_count - 1)) == (float)m_focus
                  ? m_focus
                  : 0;
}

void MenuSystem::panel(UIRenderer& ui, float x, float y, float w, float h, float alpha) {
    ui.rect(x, y, w, h, kPanel, alpha);
    ui.rect(x, y, w, 3.0f, kAccent, 0.85f);
}

void MenuSystem::title(UIRenderer& ui, const std::string& text, float y) {
    const float cx = (float)ui.screenWidth() * 0.5f;
    ui.textShadow(text, cx, y, 46.0f, kInk, 1.0f, TextAlign::Center);
    ui.rect(cx - 120.0f, y + 58.0f, 240.0f, 3.0f, kAccent, 0.9f);
}

bool MenuSystem::button(UIRenderer& ui, Input& input, int id, const std::string& label, float x,
                        float y, float w, float h) {
    const glm::vec2 m = input.mousePosition();
    const bool hovered = m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h;
    if (hovered) m_focus = id;
    const bool focused = (m_focus == id);

    const float pulse = 0.5f + 0.5f * std::sin(m_time * 4.0f);
    ui.rect(x, y, w, h, focused ? glm::vec3(0.16f, 0.17f, 0.20f) : glm::vec3(0.07f, 0.075f, 0.09f),
            focused ? 0.95f : 0.75f);
    if (focused) {
        ui.rect(x, y, 5.0f, h, kAccent, 0.75f + 0.25f * pulse);
        ui.rectOutline(x, y, w, h, 1.5f, kAccent, 0.55f);
    }
    ui.text(label, x + 26.0f, y + h * 0.5f - 13.0f, 26.0f, focused ? kInk : kDim, 1.0f);

    const bool clicked = hovered && input.mousePressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool entered = focused && (input.keyPressed(GLFW_KEY_ENTER) ||
                                     input.keyPressed(GLFW_KEY_SPACE) ||
                                     input.keyPressed(GLFW_KEY_KP_ENTER));
    return clicked || entered;
}

bool MenuSystem::option(UIRenderer& ui, Input& input, int id, const std::string& label,
                        const std::string& value, float x, float y, float w, float h,
                        int& delta) {
    delta = 0;
    const glm::vec2 m = input.mousePosition();
    const bool hovered = m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h;
    if (hovered) m_focus = id;
    const bool focused = (m_focus == id);

    ui.rect(x, y, w, h, focused ? glm::vec3(0.15f, 0.16f, 0.19f) : glm::vec3(0.065f, 0.07f, 0.085f),
            focused ? 0.95f : 0.72f);
    if (focused) ui.rect(x, y, 5.0f, h, kAccent, 0.9f);
    ui.text(label, x + 26.0f, y + h * 0.5f - 11.0f, 22.0f, focused ? kInk : kDim, 1.0f);

    const float arrowL = x + w - 190.0f;
    const float arrowR = x + w - 30.0f;
    ui.text("<", arrowL, y + h * 0.5f - 11.0f, 22.0f, focused ? kAccent : kDim, 1.0f);
    ui.text(">", arrowR, y + h * 0.5f - 11.0f, 22.0f, focused ? kAccent : kDim, 1.0f,
            TextAlign::Right);
    ui.text(value, (arrowL + arrowR) * 0.5f, y + h * 0.5f - 11.0f, 22.0f, kInk, 1.0f,
            TextAlign::Center);

    if (focused) {
        if (input.keyPressed(GLFW_KEY_LEFT) || input.keyPressed(GLFW_KEY_A)) delta = -1;
        if (input.keyPressed(GLFW_KEY_RIGHT) || input.keyPressed(GLFW_KEY_D)) delta = 1;
    }
    if (hovered && input.mousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
        delta = (m.x > (arrowL + arrowR) * 0.5f) ? 1 : -1;
    }
    return delta != 0;
}

// ==================================================================== screens
MenuAction MenuSystem::drawMainMenu(UIRenderer& ui, Input& input) {
    beginScreen(3, input);
    const float W = (float)ui.screenWidth();
    const float H = (float)ui.screenHeight();

    // Cinematic letterbox + vignette panels.
    ui.rect(0.0f, 0.0f, W, 96.0f, glm::vec3(0.0f), 0.55f);
    ui.rect(0.0f, H - 60.0f, W, 60.0f, glm::vec3(0.0f), 0.55f);

    ui.textShadow("DUA XE MAY HUT BUI", 60.0f, H * 0.26f, 62.0f, kInk);
    ui.rect(62.0f, H * 0.26f + 76.0f, 330.0f, 4.0f, kAccent, 0.95f);
    ui.text("VACUUM RACING 3D", 64.0f, H * 0.26f + 90.0f, 24.0f, kAccent, 0.95f);
    ui.text("C++ / OpenGL 3.3 - Computer Graphics Project", 64.0f, H * 0.26f + 120.0f, 17.0f,
            kDim, 0.9f);

    const float bx = 60.0f;
    float       by = H * 0.52f;
    const float bw = 330.0f;
    const float bh = 58.0f;

    MenuAction action = MenuAction::None;
    if (button(ui, input, 0, "START RACE", bx, by, bw, bh)) action = MenuAction::GoColorSelect;
    by += bh + 12.0f;
    if (button(ui, input, 1, "SETTINGS", bx, by, bw, bh)) action = MenuAction::GoSettings;
    by += bh + 12.0f;
    if (button(ui, input, 2, "EXIT", bx, by, bw, bh)) action = MenuAction::Exit;

    ui.text("W/S or mouse to navigate   -   ENTER to confirm", W - 30.0f, H - 40.0f, 17.0f, kDim,
            0.85f, TextAlign::Right);
    return action;
}

MenuAction MenuSystem::drawColorSelect(UIRenderer& ui, Input& input, Settings& settings) {
    beginScreen(8, input);
    const float W = (float)ui.screenWidth();
    const float H = (float)ui.screenHeight();

    ui.rect(0.0f, 0.0f, W, 92.0f, glm::vec3(0.0f), 0.5f);
    ui.rect(0.0f, H - 210.0f, W, 210.0f, glm::vec3(0.0f), 0.55f);
    ui.textShadow("CHOOSE YOUR COLOUR", W * 0.5f, 24.0f, 40.0f, kInk, 1.0f, TextAlign::Center);

    const float swatch = 96.0f;
    const float gap    = 18.0f;
    const float total  = 6.0f * swatch + 5.0f * gap;
    const float sx     = W * 0.5f - total * 0.5f;
    const float sy     = H - 178.0f;

    MenuAction action = MenuAction::None;
    for (int i = 0; i < 6; ++i) {
        const float x = sx + (float)i * (swatch + gap);
        const glm::vec2 m = input.mousePosition();
        const bool hovered = m.x >= x && m.x <= x + swatch && m.y >= sy && m.y <= sy + swatch;
        if (hovered) m_focus = i;
        const bool focused  = (m_focus == i);
        const bool selected = (settings.carColor == i);

        ui.rect(x - 4.0f, sy - 4.0f, swatch + 8.0f, swatch + 8.0f,
                selected ? kAccent : glm::vec3(0.15f), selected ? 0.95f : 0.6f);
        ui.rect(x, sy, swatch, swatch,
                glm::vec3(kCarColors[i].r, kCarColors[i].g, kCarColors[i].b), 1.0f);
        if (focused) ui.rectOutline(x - 4.0f, sy - 4.0f, swatch + 8.0f, swatch + 8.0f, 3.0f, kInk,
                                    0.9f);
        ui.text(kCarColors[i].name, x + swatch * 0.5f, sy + swatch + 10.0f, 17.0f,
                selected ? kInk : kDim, 1.0f, TextAlign::Center);

        if ((hovered && input.mousePressed(GLFW_MOUSE_BUTTON_LEFT)) ||
            (focused && (input.keyPressed(GLFW_KEY_ENTER) || input.keyPressed(GLFW_KEY_SPACE)))) {
            settings.carColor = i;
        }
    }
    // Left/right also moves through the swatches.
    if (m_focus < 6) {
        if (input.keyPressed(GLFW_KEY_LEFT) || input.keyPressed(GLFW_KEY_A)) {
            m_focus = (m_focus + 5) % 6;
        }
        if (input.keyPressed(GLFW_KEY_RIGHT) || input.keyPressed(GLFW_KEY_D)) {
            m_focus = (m_focus + 1) % 6;
        }
    }

    if (button(ui, input, 6, "START RACE", W * 0.5f - 250.0f, H - 62.0f, 240.0f, 48.0f)) {
        action = MenuAction::StartRace;
    }
    if (button(ui, input, 7, "BACK", W * 0.5f + 10.0f, H - 62.0f, 240.0f, 48.0f)) {
        action = MenuAction::GoMainMenu;
    }
    if (input.keyPressed(GLFW_KEY_ESCAPE)) action = MenuAction::GoMainMenu;
    return action;
}

MenuAction MenuSystem::drawSettings(UIRenderer& ui, Input& input, Settings& settings) {
    beginScreen(8, input);
    const float W = (float)ui.screenWidth();
    const float H = (float)ui.screenHeight();

    ui.rect(0.0f, 0.0f, W, H, glm::vec3(0.0f), 0.62f);
    title(ui, "SETTINGS", 62.0f);

    const float w = 620.0f;
    const float x = W * 0.5f - w * 0.5f;
    float       y = 170.0f;
    const float h = 48.0f;
    const float gap = 10.0f;

    MenuAction action = MenuAction::None;
    int  delta = 0;
    char buf[64];

    int resIndex = Settings::closestResolution(settings.windowWidth, settings.windowHeight);
    if (option(ui, input, 0, "RESOLUTION", Settings::resolutionLabel(resIndex), x, y, w, h,
               delta)) {
        resIndex = (resIndex + delta + Settings::resolutionCount()) % Settings::resolutionCount();
        Settings::resolutionSize(resIndex, settings.windowWidth, settings.windowHeight);
        action = MenuAction::ApplyVideo;
    }
    y += h + gap;

    if (option(ui, input, 1, "FULLSCREEN", settings.fullscreen ? "ON" : "OFF", x, y, w, h,
               delta)) {
        settings.fullscreen = !settings.fullscreen;
        action = MenuAction::ApplyVideo;
    }
    y += h + gap;

    if (option(ui, input, 2, "V-SYNC", settings.vsync ? "ON" : "OFF", x, y, w, h, delta)) {
        settings.vsync = !settings.vsync;
        action = MenuAction::ApplyVideo;
    }
    y += h + gap;

    const char* qualityNames[3] = {"LOW", "MEDIUM", "HIGH"};
    if (option(ui, input, 3, "GRAPHICS QUALITY", qualityNames[settings.quality], x, y, w, h,
               delta)) {
        settings.quality = std::min(2, std::max(0, settings.quality + delta));
        action = MenuAction::ApplyVideo;
    }
    y += h + gap;

    std::snprintf(buf, sizeof(buf), "%d%%", (int)(settings.masterVolume * 100.0f));
    if (option(ui, input, 4, "MASTER VOLUME", buf, x, y, w, h, delta)) {
        settings.masterVolume = math::clampf(settings.masterVolume + 0.05f * (float)delta, 0.0f,
                                             1.0f);
    }
    y += h + gap;

    std::snprintf(buf, sizeof(buf), "%d%%", (int)(settings.musicVolume * 100.0f));
    if (option(ui, input, 5, "MUSIC VOLUME", buf, x, y, w, h, delta)) {
        settings.musicVolume = math::clampf(settings.musicVolume + 0.05f * (float)delta, 0.0f,
                                            1.0f);
    }
    y += h + gap;

    std::snprintf(buf, sizeof(buf), "%d%%", (int)(settings.sfxVolume * 100.0f));
    if (option(ui, input, 6, "SFX VOLUME", buf, x, y, w, h, delta)) {
        settings.sfxVolume = math::clampf(settings.sfxVolume + 0.05f * (float)delta, 0.0f, 1.0f);
    }
    y += h + gap * 2.0f;

    if (button(ui, input, 7, "BACK", x, y, w, h)) action = MenuAction::Back;
    if (input.keyPressed(GLFW_KEY_ESCAPE)) action = MenuAction::Back;

    ui.text("Settings are saved automatically.", W * 0.5f, y + h + 18.0f, 17.0f, kDim, 0.85f,
            TextAlign::Center);
    return action;
}

MenuAction MenuSystem::drawPause(UIRenderer& ui, Input& input) {
    beginScreen(4, input);
    const float W = (float)ui.screenWidth();
    const float H = (float)ui.screenHeight();

    ui.rect(0.0f, 0.0f, W, H, glm::vec3(0.0f), 0.58f);
    const float w = 380.0f;
    const float h = 340.0f;
    const float x = W * 0.5f - w * 0.5f;
    const float y = H * 0.5f - h * 0.5f;
    panel(ui, x, y, w, h, 0.9f);
    ui.textShadow("PAUSED", W * 0.5f, y + 26.0f, 40.0f, kInk, 1.0f, TextAlign::Center);

    MenuAction action = MenuAction::None;
    float by = y + 96.0f;
    const float bw = w - 56.0f;
    const float bh = 50.0f;
    if (button(ui, input, 0, "RESUME", x + 28.0f, by, bw, bh)) action = MenuAction::Resume;
    by += bh + 10.0f;
    if (button(ui, input, 1, "RESTART RACE", x + 28.0f, by, bw, bh)) action = MenuAction::Restart;
    by += bh + 10.0f;
    if (button(ui, input, 2, "MAIN MENU", x + 28.0f, by, bw, bh)) action = MenuAction::GoMainMenu;
    by += bh + 10.0f;
    if (button(ui, input, 3, "EXIT GAME", x + 28.0f, by, bw, bh)) action = MenuAction::Exit;
    return action;
}

MenuAction MenuSystem::drawResults(UIRenderer& ui, Input& input, const RaceManager& race,
                                   const std::vector<Vehicle>& field) {
    beginScreen(3, input);
    const float W = (float)ui.screenWidth();
    const float H = (float)ui.screenHeight();
    ui.rect(0.0f, 0.0f, W, H, glm::vec3(0.0f), 0.66f);

    const int pos = race.playerPosition();
    const bool win = (pos == 1);
    if (win) {
        const float pulse = 0.55f + 0.45f * std::sin(m_time * 3.4f);
        ui.textShadow("VICTORY", W * 0.5f, 48.0f, 74.0f,
                      glm::mix(glm::vec3(1.0f, 0.85f, 0.25f), kInk, pulse), 1.0f,
                      TextAlign::Center);
    } else {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "FINISHED  P%d", pos);
        ui.textShadow(buf, W * 0.5f, 52.0f, 58.0f, kInk, 1.0f, TextAlign::Center);
    }

    // ---- classification table
    const float tw = 720.0f;
    const float tx = W * 0.5f - tw * 0.5f;
    float       ty = 150.0f;
    panel(ui, tx, ty, tw, 44.0f + 30.0f * (float)race.results().size(), 0.82f);
    ui.text("POS", tx + 24.0f, ty + 14.0f, 18.0f, kAccent);
    ui.text("DRIVER", tx + 90.0f, ty + 14.0f, 18.0f, kAccent);
    ui.text("TOTAL TIME", tx + 330.0f, ty + 14.0f, 18.0f, kAccent);
    ui.text("BEST LAP", tx + 500.0f, ty + 14.0f, 18.0f, kAccent);
    ui.text("GAP", tx + 640.0f, ty + 14.0f, 18.0f, kAccent);
    ty += 44.0f;

    for (const ResultRow& r : race.results()) {
        const bool me = (r.name == std::string("PLAYER"));
        const glm::vec3 col = me ? kAccent : glm::vec3(0.88f);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d", r.position);
        ui.text(buf, tx + 24.0f, ty + 6.0f, 19.0f, col);
        ui.text(r.name, tx + 90.0f, ty + 6.0f, 19.0f, col);
        ui.text(formatTime(r.totalTime), tx + 330.0f, ty + 6.0f, 19.0f, col);
        ui.text(formatTime(r.bestLap), tx + 500.0f, ty + 6.0f, 19.0f, col);
        if (r.position == 1) {
            ui.text("-", tx + 640.0f, ty + 6.0f, 19.0f, col);
        } else {
            std::snprintf(buf, sizeof(buf), "+%.2f", r.gap);
            ui.text(buf, tx + 640.0f, ty + 6.0f, 19.0f, col);
        }
        ty += 30.0f;
    }

    // ---- player's lap breakdown
    const Vehicle* player = nullptr;
    for (const Vehicle& v : field) {
        if (!v.isAI()) player = &v;
    }
    if (player) {
        ty += 26.0f;
        for (int i = 0; i < 3; ++i) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "LAP %d", i + 1);
            ui.text(buf, tx + 24.0f, ty, 19.0f, kDim);
            ui.text(formatTime(player->race.lapTimes[i]), tx + 130.0f, ty, 19.0f, kInk);
            ty += 26.0f;
        }
    }

    MenuAction action = MenuAction::None;
    const float bw = 220.0f;
    const float by = H - 92.0f;
    if (button(ui, input, 0, "RESTART", W * 0.5f - bw * 1.55f, by, bw, 50.0f)) {
        action = MenuAction::Restart;
    }
    if (button(ui, input, 1, "MAIN MENU", W * 0.5f - bw * 0.5f, by, bw, 50.0f)) {
        action = MenuAction::GoMainMenu;
    }
    if (button(ui, input, 2, "EXIT", W * 0.5f + bw * 0.55f, by, bw, 50.0f)) {
        action = MenuAction::Exit;
    }
    return action;
}

MenuAction MenuSystem::drawLoading(UIRenderer& ui, float progress) {
    const float W = (float)ui.screenWidth();
    const float H = (float)ui.screenHeight();
    ui.rect(0.0f, 0.0f, W, H, glm::vec3(0.03f, 0.035f, 0.045f), 1.0f);
    ui.textShadow("VACUUM RACING 3D", W * 0.5f, H * 0.40f, 46.0f, kInk, 1.0f, TextAlign::Center);
    ui.text("BUILDING THE CIRCUIT...", W * 0.5f, H * 0.40f + 66.0f, 20.0f, kDim, 0.9f,
            TextAlign::Center);

    const float bw = 520.0f;
    const float bx = W * 0.5f - bw * 0.5f;
    const float by = H * 0.56f;
    ui.rect(bx, by, bw, 12.0f, glm::vec3(0.10f), 0.9f);
    ui.rect(bx, by, bw * math::saturate(progress), 12.0f, kAccent, 1.0f);
    return MenuAction::None;
}

} // namespace vr
