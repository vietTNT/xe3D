#include "HUD.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../Managers/RaceManager.h"
#include "../Track/Track.h"
#include "../Utilities/MathUtils.h"
#include "../Vehicle/Vehicle.h"
#include "UIRenderer.h"

namespace vr {
namespace {

const glm::vec3 kInk(0.94f, 0.95f, 0.97f);
const glm::vec3 kAccent(0.98f, 0.34f, 0.10f);
const glm::vec3 kPanel(0.04f, 0.045f, 0.055f);

std::string formatTime(float seconds) {
    char buf[24];
    math::formatLapTime(seconds, buf, sizeof(buf));
    return std::string(buf);
}

} // namespace

void HUD::draw(UIRenderer& ui, const Vehicle& player, const RaceManager& race, const Track& track,
               const std::vector<Vehicle>& field, float fps, bool showFps, float dt) {
    m_shownSpeed = math::damp(m_shownSpeed, std::fabs(player.speedKmh()), 16.0f, dt);
    m_shownRpm   = math::damp(m_shownRpm, player.engineRpm(), 14.0f, dt);

    drawSpeedPanel(ui, player);
    drawRacePanel(ui, player, race);
    drawStandings(ui, field);
    drawMinimap(ui, track, player, field);
    drawCountdown(ui, race);
    drawBanner(ui, race);

    if (showFps) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f FPS", fps);
        ui.textShadow(buf, (float)ui.screenWidth() - 16.0f, 12.0f, 18.0f,
                      glm::vec3(0.65f, 1.0f, 0.7f), 0.9f, TextAlign::Right);
    }
}

void HUD::drawSpeedPanel(UIRenderer& ui, const Vehicle& player) {
    const float W = (float)ui.screenWidth();
    const float H = (float)ui.screenHeight();
    const float w = 300.0f;
    const float h = 116.0f;
    const float x = W - w - 26.0f;
    const float y = H - h - 26.0f;

    ui.rect(x, y, w, h, kPanel, 0.55f);
    ui.rect(x, y, 4.0f, h, kAccent, 0.9f);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%3d", (int)(m_shownSpeed + 0.5f));
    ui.textShadow(buf, x + w - 82.0f, y + 18.0f, 62.0f, kInk, 1.0f, TextAlign::Right);
    ui.text("KM/H", x + w - 66.0f, y + 54.0f, 20.0f, glm::vec3(0.7f), 0.9f, TextAlign::Left);

    // Gear.
    std::snprintf(buf, sizeof(buf), "%d", player.gear());
    ui.textShadow(player.speed() < -0.4f ? "R" : buf, x + 42.0f, y + 24.0f, 46.0f, kAccent, 1.0f,
                  TextAlign::Center);
    ui.text("GEAR", x + 42.0f, y + 74.0f, 15.0f, glm::vec3(0.62f), 0.85f, TextAlign::Center);

    // RPM bar with a red line at the top of the range.
    const float barX = x + 14.0f;
    const float barY = y + h - 16.0f;
    const float barW = w - 28.0f;
    ui.rect(barX, barY, barW, 7.0f, glm::vec3(0.10f), 0.8f);
    const float t = math::saturate((m_shownRpm - 900.0f) / 7400.0f);
    const glm::vec3 col = t > 0.86f ? glm::vec3(1.0f, 0.16f, 0.12f)
                                    : glm::mix(glm::vec3(0.35f, 0.85f, 0.45f), kAccent, t);
    ui.rect(barX, barY, barW * t, 7.0f, col, 0.96f);
    for (int i = 1; i < 8; ++i) {
        ui.rect(barX + barW * (float)i / 8.0f, barY, 1.0f, 7.0f, glm::vec3(0.0f), 0.45f);
    }
}

void HUD::drawRacePanel(UIRenderer& ui, const Vehicle& player, const RaceManager& race) {
    const float x = 26.0f;
    const float y = 22.0f;
    const float w = 250.0f;
    const float h = 104.0f;

    ui.rect(x, y, w, h, kPanel, 0.55f);
    ui.rect(x, y, 4.0f, h, kAccent, 0.9f);

    char buf[64];
    const int lap = math::clampf((float)player.race.lap + 1.0f, 1.0f,
                                 (float)race.totalLaps()) == 0.0f
                        ? 1
                        : std::min(std::max(player.race.lap + 1, 1), race.totalLaps());
    std::snprintf(buf, sizeof(buf), "%d/%d", lap, race.totalLaps());
    ui.text("LAP", x + 16.0f, y + 12.0f, 16.0f, glm::vec3(0.62f), 0.9f);
    ui.textShadow(buf, x + 16.0f, y + 28.0f, 34.0f, kInk);

    std::snprintf(buf, sizeof(buf), "%d/6", player.race.position);
    ui.text("POS", x + 128.0f, y + 12.0f, 16.0f, glm::vec3(0.62f), 0.9f);
    ui.textShadow(buf, x + 128.0f, y + 28.0f, 34.0f, kAccent);

    ui.text("TIME  " + formatTime(player.race.totalTime), x + 16.0f, y + 68.0f, 17.0f,
            glm::vec3(0.86f), 0.95f);
    ui.text("BEST  " + formatTime(player.race.bestLap), x + 16.0f, y + 85.0f, 17.0f,
            glm::vec3(0.55f, 0.85f, 1.0f), 0.95f);
}

void HUD::drawStandings(UIRenderer& ui, const std::vector<Vehicle>& field) {
    const float x = 26.0f;
    const float y = 140.0f;
    const float rowH = 20.0f;

    std::vector<const Vehicle*> order;
    order.reserve(field.size());
    for (const Vehicle& v : field) order.push_back(&v);
    std::sort(order.begin(), order.end(),
              [](const Vehicle* a, const Vehicle* b) { return a->race.position < b->race.position; });

    ui.rect(x, y, 190.0f, rowH * (float)order.size() + 10.0f, kPanel, 0.42f);
    for (size_t i = 0; i < order.size(); ++i) {
        const Vehicle* v = order[i];
        const float ry = y + 6.0f + (float)i * rowH;
        char buf[48];
        std::snprintf(buf, sizeof(buf), "%d", v->race.position);
        const glm::vec3 col = v->isAI() ? glm::vec3(0.80f) : kAccent;
        ui.text(buf, x + 14.0f, ry, 15.0f, col, 0.95f);
        ui.rect(x + 30.0f, ry + 3.0f, 9.0f, 9.0f, v->color(), 0.95f);
        ui.text(v->name(), x + 46.0f, ry, 15.0f, col, 0.95f);
    }
}

void HUD::drawMinimap(UIRenderer& ui, const Track& track, const Vehicle& player,
                      const std::vector<Vehicle>& field) {
    const float size = 190.0f;
    const float pad  = 26.0f;
    const float x    = pad;
    const float y    = (float)ui.screenHeight() - size - pad;

    ui.rect(x, y, size, size, kPanel, 0.48f);
    ui.rectOutline(x, y, size, size, 2.0f, glm::vec3(0.25f), 0.6f);

    const glm::vec2 mn = track.minimapMin();
    const glm::vec2 mx = track.minimapMax();
    const glm::vec2 span = glm::max(mx - mn, glm::vec2(1.0f));
    const float scale = (size - 26.0f) / std::max(span.x, span.y);
    const glm::vec2 offset(x + size * 0.5f - (mn.x + span.x * 0.5f) * scale,
                           y + size * 0.5f - (mn.y + span.y * 0.5f) * scale);

    auto project = [&](const glm::vec2& p) { return glm::vec2(p.x * scale, p.y * scale) + offset; };

    const std::vector<glm::vec2>& pts = track.minimapPoints();
    for (size_t i = 0; i < pts.size(); ++i) {
        const glm::vec2 a = project(pts[i]);
        const glm::vec2 b = project(pts[(i + 1) % pts.size()]);
        ui.line(a, b, 3.0f, glm::vec3(0.62f, 0.64f, 0.70f), 0.85f);
    }

    // Start/finish marker.
    const TrackSample s = track.sampleAt(track.startLineDistance());
    const glm::vec2 sp = project(glm::vec2(s.position.x, s.position.z));
    ui.rect(sp.x - 4.0f, sp.y - 4.0f, 8.0f, 8.0f, glm::vec3(0.95f), 0.95f);

    for (const Vehicle& v : field) {
        const glm::vec2 p = project(glm::vec2(v.position().x, v.position().z));
        const bool me = !v.isAI();
        ui.disc(p, me ? 5.0f : 4.0f, me ? kAccent : v.color(), 1.0f, 10);
        if (me) {
            const glm::vec3 f = v.forward();
            ui.line(p, p + glm::vec2(f.x, f.z) * 11.0f, 2.0f, kAccent, 0.9f);
        }
    }
    (void)player;
}

void HUD::drawCountdown(UIRenderer& ui, const RaceManager& race) {
    if (race.phase() != RacePhase::Countdown) return;
    const float t = race.countdown();
    const int   n = (int)std::ceil(t - 1.2f);
    const float W = (float)ui.screenWidth() * 0.5f;
    const float Y = (float)ui.screenHeight() * 0.30f;

    if (n >= 1 && n <= 3) {
        const float frac = 1.0f - (t - 1.2f - std::floor(t - 1.2f));
        const float sz   = 150.0f + 40.0f * (1.0f - frac);
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d", n);
        ui.textShadow(buf, W, Y, sz, kInk, math::saturate(1.4f - frac * 0.8f), TextAlign::Center);
    }
    ui.textShadow("GET READY", W, Y + 190.0f, 26.0f, glm::vec3(0.75f), 0.8f, TextAlign::Center);
}

void HUD::drawBanner(UIRenderer& ui, const RaceManager& race) {
    if (race.bannerAlpha() <= 0.0f || race.banner().empty()) return;
    const float a = math::saturate(race.bannerAlpha());
    ui.textShadow(race.banner(), (float)ui.screenWidth() * 0.5f,
                  (float)ui.screenHeight() * 0.22f, 58.0f, kAccent, a, TextAlign::Center);
}

} // namespace vr
