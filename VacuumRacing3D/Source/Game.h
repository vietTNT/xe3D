// -----------------------------------------------------------------------------
//  Game.h - application shell and state machine.
//
//  Main Menu -> Colour Select -> Countdown -> Race -> Results, with a pause
//  menu available at any time during the race.
// -----------------------------------------------------------------------------
#pragma once

#include <vector>

#include "AI/AIController.h"
#include "Audio/Audio.h"
#include "Camera/ChaseCamera.h"
#include "Core/Window.h"
#include "Graphics/ParticleSystem.h"
#include "Graphics/Renderer.h"
#include "Managers/RaceManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/Settings.h"
#include "Track/Environment.h"
#include "Track/Track.h"
#include "UI/HUD.h"
#include "UI/Menu.h"
#include "UI/UIRenderer.h"
#include "Vehicle/CarModel.h"
#include "Vehicle/Vehicle.h"

namespace vr {

enum class GameState { Loading, MainMenu, ColorSelect, SettingsMenu, Race, Paused, Results };

class Game {
public:
    bool init(int argc, char** argv);
    void run();
    void shutdown();

private:
    // ---- flow
    void stepLoading();
    void startRace();
    void updateRace(float dt);
    void updateMenuCamera(float dt);
    void applyVideoSettings();
    void handleGlobalKeys();

    // ---- rendering
    void renderFrame(float dt);
    void submitWorld();
    void drawInterface(float dt);

    VehicleInput readPlayerInput() const;

    Window          m_window;
    Renderer        m_renderer;
    ResourceManager m_resources;
    Track           m_track;
    Environment     m_environment;
    CarModel        m_carModel;
    ParticleSystem  m_particles;
    ChaseCamera     m_camera;
    UIRenderer      m_ui;
    HUD             m_hud;
    MenuSystem      m_menu;
    RaceManager     m_race;
    Settings        m_settings;
    Audio           m_audio;
    DirectionalLight m_light;

    std::vector<Vehicle>      m_cars;
    std::vector<AIController> m_ai;
    std::vector<VehicleInput> m_aiInputs;

    GameState m_state       = GameState::Loading;
    GameState m_returnState = GameState::MainMenu;
    GameState m_prevState   = GameState::Loading;
    int       m_loadStage   = 0;
    bool      m_worldReady  = false;   ///< true once the staged load has finished
    float     m_loadProgress = 0.0f;
    bool      m_running     = true;
    float     m_time        = 0.0f;
    float     m_fps         = 60.0f;
    float     m_flash       = 0.0f;
    int       m_lastLightsLit = 0;
    float     m_aiTimer     = 0.0f;

    // Splash screen timing/state
    float m_splashTimer = 0.0f;
    float m_splashAlpha = 0.0f;
    bool  m_splashDone  = false;

    static constexpr int kCarCount   = 6;
    static constexpr int kPlayerSlot = 5;   ///< the player starts at the back
};

} // namespace vr
