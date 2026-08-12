// -----------------------------------------------------------------------------
//  HUD.h - in-race overlay: speed, gear, RPM, lap, position, timing, minimap.
// -----------------------------------------------------------------------------
#pragma once

#include <vector>

namespace vr {

class RaceManager;
class Track;
class UIRenderer;
class Vehicle;

class HUD {
public:
    void draw(UIRenderer& ui, const Vehicle& player, const RaceManager& race, const Track& track,
              const std::vector<Vehicle>& field, float fps, bool showFps, float dt);

private:
    void drawSpeedPanel(UIRenderer& ui, const Vehicle& player);
    void drawRacePanel(UIRenderer& ui, const Vehicle& player, const RaceManager& race);
    void drawStandings(UIRenderer& ui, const std::vector<Vehicle>& field);
    void drawMinimap(UIRenderer& ui, const Track& track, const Vehicle& player,
                     const std::vector<Vehicle>& field);
    void drawCountdown(UIRenderer& ui, const RaceManager& race);
    void drawBanner(UIRenderer& ui, const RaceManager& race);

    float m_shownSpeed = 0.0f;
    float m_shownRpm   = 0.0f;
};

} // namespace vr
