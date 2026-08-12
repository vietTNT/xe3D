// -----------------------------------------------------------------------------
//  Menu.h - main menu, car colour picker, settings, pause and results screens.
//  Everything is immediate mode: the screens are redrawn every frame and return
//  the action the player triggered.
// -----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace vr {

class Input;
class RaceManager;
class UIRenderer;
class Vehicle;
struct Settings;

enum class MenuAction {
    None,
    StartRace,
    GoColorSelect,
    GoSettings,
    GoMainMenu,
    Resume,
    Restart,
    Exit,
    Back,
    ApplyVideo
};

class MenuSystem {
public:
    void tick(float dt) { m_time += dt; }

    MenuAction drawMainMenu(UIRenderer& ui, Input& input);
    MenuAction drawColorSelect(UIRenderer& ui, Input& input, Settings& settings);
    MenuAction drawSettings(UIRenderer& ui, Input& input, Settings& settings);
    MenuAction drawPause(UIRenderer& ui, Input& input);
    MenuAction drawResults(UIRenderer& ui, Input& input, const RaceManager& race,
                           const std::vector<Vehicle>& field);
    MenuAction drawLoading(UIRenderer& ui, float progress);

    void resetFocus() { m_focus = 0; }

private:
    void  beginScreen(int itemCount, Input& input);
    bool  button(UIRenderer& ui, Input& input, int id, const std::string& label, float x, float y,
                 float w, float h);
    bool  option(UIRenderer& ui, Input& input, int id, const std::string& label,
                 const std::string& value, float x, float y, float w, float h, int& delta);
    void  panel(UIRenderer& ui, float x, float y, float w, float h, float alpha = 0.72f);
    void  title(UIRenderer& ui, const std::string& text, float y);

    float m_time  = 0.0f;
    int   m_focus = 0;
    int   m_count = 0;
};

} // namespace vr
