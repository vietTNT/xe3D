#include "Game.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <glm/gtc/matrix_transform.hpp>

#include "Core/Paths.h"
#include "Utilities/MathUtils.h"

namespace vr {
namespace {

const char* kDriverNames[6] = {"PLAYER", "R. VOSS", "K. MORI", "A. SILVA", "T. NGUYEN",
                               "L. FISCHER"};

const glm::vec3 kAIColors[5] = {{0.05f, 0.16f, 0.55f}, {0.90f, 0.70f, 0.05f},
                                {0.05f, 0.42f, 0.20f}, {0.70f, 0.72f, 0.75f},
                                {0.55f, 0.10f, 0.55f}};

} // namespace

bool Game::init(int argc, char** argv) {
    (void)argc;
    paths::initFromExecutable(argv && argv[0] ? argv[0] : nullptr);
    m_settings.load();

    WindowConfig cfg;
    cfg.width      = m_settings.windowWidth;
    cfg.height     = m_settings.windowHeight;
    cfg.fullscreen = m_settings.fullscreen;
    cfg.vsync      = m_settings.vsync;
    if (!m_window.create(cfg)) return false;

    if (!m_renderer.init(m_window.framebufferWidth(), m_window.framebufferHeight(),
                         m_settings.quality)) {
        std::fprintf(stderr, "[Game] renderer initialisation failed\n");
        return false;
    }
    if (!m_ui.init()) return false;

    // Late afternoon sun: low, warm and slightly to the side.
    m_light.direction     = glm::normalize(glm::vec3(-0.46f, -0.40f, -0.32f));
    m_light.color         = glm::vec3(1.0f, 0.80f, 0.58f);
    m_light.intensity     = 3.25f;
    m_light.ambientSky    = glm::vec3(0.28f, 0.35f, 0.48f);
    m_light.ambientGround = glm::vec3(0.17f, 0.15f, 0.12f);
    m_renderer.fogColor   = glm::vec3(0.72f, 0.75f, 0.80f);
    m_renderer.fogDensity = 0.00072f;   // light haze: the ridges stay visible

    m_audio.init(m_settings);
    m_camera.setMode((CameraMode)math::clampf((float)m_settings.cameraMode, 0.0f, 2.0f));

    // Allocate the field immediately: the loading screen renders before the
    // world is built, and nothing may index into an empty vector.
    m_cars.resize(kCarCount);
    m_ai.resize(kCarCount);
    m_aiInputs.assign(kCarCount, VehicleInput{});
    int aiIndex = 0;
    for (int i = 0; i < kCarCount; ++i) {
        const bool isPlayer = (i == 0);
        const glm::vec3 color = isPlayer ? glm::vec3(kCarColors[m_settings.carColor].r,
                                                     kCarColors[m_settings.carColor].g,
                                                     kCarColors[m_settings.carColor].b)
                                         : kAIColors[aiIndex++];
        m_cars[(size_t)i].configure(color, !isPlayer, i, kDriverNames[i]);
        if (!isPlayer) m_ai[(size_t)i].init(i, 0.84f + 0.026f * (float)i, 1337u);
    }

    m_state = GameState::Loading;
    return true;
}

// ================================================================== staged load
void Game::stepLoading() {
    switch (m_loadStage) {
        case 0:
            m_resources.loadAll(m_settings.quality);
            m_loadProgress = 0.20f;
            break;
        case 1:
            m_track.build(m_resources, m_settings.quality);
            m_loadProgress = 0.55f;
            break;
        case 2:
            m_environment.build(m_track, m_resources, m_settings.quality);
            m_loadProgress = 0.78f;
            break;
        case 3:
            m_carModel.build(m_resources, m_settings.quality);
            std::printf("[Car] %d triangles\n", m_carModel.triangleCount());
            m_loadProgress = 0.90f;
            break;
        case 4:
            m_particles.init(m_resources, m_settings.quality);
            m_loadProgress = 1.0f;
            break;
        default:
            m_worldReady = true;
            std::printf("[Game] world ready - entering the main menu\n");
            Vehicle::runPhysicsDiagnosticTests(m_track);
            // Apply the player's saved colour to livery slot 0 immediately.
            {
                const glm::vec3 pc(kCarColors[m_settings.carColor].r,
                                   kCarColors[m_settings.carColor].g,
                                   kCarColors[m_settings.carColor].b);
                m_resources.regeneratePlayerLivery(pc, 12);
                m_cars[0].configure(pc, false, 0, kDriverNames[0]);
                m_lastAppliedColor = m_settings.carColor;
            }
            m_state = GameState::MainMenu;
            m_menu.resetFocus();
            m_audio.setMusic(MusicMood::Menu);
            // Park the player's car by the pit straight for the menu backdrop.
            for (int i = 0; i < kCarCount; ++i) m_cars[(size_t)i].reset(m_track, i);
            m_camera.reset(m_cars[0]);
            return;
    }
    ++m_loadStage;
}

// ======================================================================== race
void Game::startRace() {
    const glm::vec3 playerColor(kCarColors[m_settings.carColor].r,
                                kCarColors[m_settings.carColor].g,
                                kCarColors[m_settings.carColor].b);
    m_resources.regeneratePlayerLivery(playerColor, 12);
    m_cars[0].configure(playerColor, false, 0, kDriverNames[0]);

    // Slot 5 is the back of the grid: the player has to work through the field.
    const int slotFor[kCarCount] = {kPlayerSlot, 0, 1, 2, 3, 4};
    for (int i = 0; i < kCarCount; ++i) {
        m_cars[(size_t)i].reset(m_track, slotFor[i]);
        if (i > 0) {
            const glm::vec3 aiColor(kAIColors[(i - 1) % 5].r, kAIColors[(i - 1) % 5].g, kAIColors[(i - 1) % 5].b);
            m_cars[(size_t)i].configure(aiColor, true, i, kDriverNames[i]);
            m_ai[(size_t)i].init(i, 0.855f + 0.022f * (float)i, 9001u + (unsigned int)i);
        }
    }
    m_particles.clear();
    m_race.begin(m_track, m_cars, 3);
    m_camera.reset(m_cars[0]);
    m_lastLightsLit = 0;
    m_flash = 0.0f;
    m_state = GameState::Race;
    m_audio.setMusic(MusicMood::Race);
    m_window.setCursorVisible(false);
}

VehicleInput Game::readPlayerInput() const {
    const Input& in = m_window.input();
    VehicleInput v;
    v.throttle = (in.keyDown(GLFW_KEY_W) || in.keyDown(GLFW_KEY_UP)) ? 1.0f : 0.0f;
    v.brake    = (in.keyDown(GLFW_KEY_S) || in.keyDown(GLFW_KEY_DOWN)) ? 1.0f : 0.0f;
    v.steer    = 0.0f;
    if (in.keyDown(GLFW_KEY_A) || in.keyDown(GLFW_KEY_LEFT)) v.steer -= 1.0f;
    if (in.keyDown(GLFW_KEY_D) || in.keyDown(GLFW_KEY_RIGHT)) v.steer += 1.0f;
    v.handbrake = in.keyDown(GLFW_KEY_SPACE);
    return v;
}

void Game::updateRace(float dt) {
    const bool green = m_race.phase() != RacePhase::Countdown;

    if (green) {
        // Refresh the AI decisions at 60 Hz, then run the physics in substeps.
        m_aiTimer += dt;
        if (m_aiTimer >= 1.0f / 60.0f) {
            for (int i = 1; i < kCarCount; ++i) {
                m_aiInputs[(size_t)i] =
                    m_ai[(size_t)i].update(m_aiTimer, m_cars[(size_t)i], m_track, m_cars);
            }
            m_aiTimer = 0.0f;
        }

        const int   steps = std::min(6, std::max(1, (int)std::ceil(dt / 0.0085f)));
        const float sub   = dt / (float)steps;
        const VehicleInput playerInput = readPlayerInput();

        for (int s = 0; s < steps; ++s) {
            m_cars[0].update(sub, playerInput, m_track, &m_particles);
            for (int i = 1; i < kCarCount; ++i) {
                m_cars[(size_t)i].update(sub, m_aiInputs[(size_t)i], m_track, &m_particles);
            }
            for (int a = 0; a < kCarCount; ++a) {
                for (int b = a + 1; b < kCarCount; ++b) {
                    Vehicle::resolvePair(m_cars[(size_t)a], m_cars[(size_t)b]);
                }
            }
            // Contact nudges cars sideways, so put every chassis back on the road.
            for (int i = 0; i < kCarCount; ++i) m_cars[(size_t)i].reseat(m_track);
        }
    } else {
        // Lights sequence: one beep per red light, a longer tone on green.
        const int lit = m_race.lightsLit();
        if (lit != m_lastLightsLit) {
            m_lastLightsLit = lit;
            m_audio.playBeep(660.0f, 0.14f);
        }
    }

    m_race.update(dt, m_track, m_cars);
    if (m_race.justStarted()) m_audio.playBeep(1050.0f, 0.55f);

    m_particles.update(dt);
    m_camera.update(dt, m_cars[0], m_track);

    // Audio telemetry from the player's car.
    const Vehicle& p = m_cars[0];
    m_audio.setEngine(p.engineRpm(), p.lastInput.throttle,
                      math::saturate(std::fabs(p.speed()) / p.tuning.maxSpeed));
    m_audio.setTyreSlip(p.slipAmount());
    if (p.collisionImpulse() > 0.25f) {
        m_audio.playImpact(math::saturate(p.collisionImpulse()));
        m_flash = std::max(m_flash, p.collisionImpulse() * 0.16f);
        m_camera.addShake(p.collisionImpulse() * 0.5f);
        m_cars[0].clearCollisionImpulse();
    }

    if (m_race.phase() == RacePhase::Finished) {
        m_state = GameState::Results;
        m_menu.resetFocus();
        m_window.setCursorVisible(true);
        m_audio.setMusic(MusicMood::Menu);
        if (m_race.playerPosition() == 1) m_audio.playVictory();
    }
}

void Game::updateMenuCamera(float dt) {
    if (!m_worldReady || m_cars.empty()) return;
    const glm::vec3 centre = m_cars[0].position() + glm::vec3(0.0f, 0.35f, 0.0f);
    m_camera.updateOrbit(dt, centre, 9.5f, 3.1f);
}

// ==================================================================== settings
void Game::applyVideoSettings() {
    m_settings.save();
    m_renderer.setQualityLevel(m_settings.quality);
    m_window.setVSync(m_settings.vsync);
    if (m_window.fullscreen() != m_settings.fullscreen) {
        m_window.setFullscreen(m_settings.fullscreen);
    } else if (!m_settings.fullscreen) {
        m_window.setSize(m_settings.windowWidth, m_settings.windowHeight);
    }
    m_audio.setVolumes(m_settings.masterVolume, m_settings.musicVolume, m_settings.sfxVolume);
}

void Game::handleGlobalKeys() {
    Input& in = m_window.input();
    if (in.keyPressed(GLFW_KEY_F1)) {
        m_settings.showFps = !m_settings.showFps;
        m_settings.save();
    }
    if (m_state == GameState::Race) {
        if (in.keyPressed(GLFW_KEY_C)) {
            m_camera.cycleMode();
            m_settings.cameraMode = (int)m_camera.mode();
            m_settings.save();
        }
        if (in.keyPressed(GLFW_KEY_ESCAPE)) {
            m_state = GameState::Paused;
            m_menu.resetFocus();
            m_window.setCursorVisible(true);
            m_audio.setMusic(MusicMood::Menu);
        }
        if (in.keyPressed(GLFW_KEY_R)) startRace();
    } else if (m_state == GameState::Paused) {
        if (in.keyPressed(GLFW_KEY_ESCAPE)) {
            m_state = GameState::Race;
            m_window.setCursorVisible(false);
            m_audio.setMusic(MusicMood::Race);
        }
    }
}

// =================================================================== rendering
void Game::submitWorld() {
    m_track.collect(m_renderer);
    m_environment.collect(m_renderer);

    // During the staged load the cars and the car model do not exist yet.
    if (!m_worldReady || m_cars.empty()) return;

    const bool racing = (m_state == GameState::Race || m_state == GameState::Paused ||
                         m_state == GameState::Results);
    if (racing) {
        m_track.collectStartLights(m_renderer, m_race.lightsLit(),
                                   m_race.phase() != RacePhase::Countdown);
        for (const Vehicle& v : m_cars) {
            m_carModel.collect(m_renderer, v.visualState());
        }
    } else {
        // Menu backdrop: only the player's car, wearing the selected colour.
        CarVisualState st = m_cars[0].visualState();
        st.bodyColor = glm::vec3(kCarColors[m_settings.carColor].r,
                                 kCarColors[m_settings.carColor].g,
                                 kCarColors[m_settings.carColor].b);
        st.headlights = true;
        m_carModel.collect(m_renderer, st);
        m_track.collectStartLights(m_renderer, 0, false);
    }
}

void Game::renderFrame(float dt) {
    if (m_window.consumeResizeFlag()) {
        m_renderer.resize(m_window.framebufferWidth(), m_window.framebufferHeight());
    }

    const CameraView view = m_camera.view(m_window.aspect());
    m_renderer.timeSeconds = m_time;
    m_renderer.beginFrame(view, m_light, m_cars.empty() ? glm::vec3(0.0f) : m_cars[0].position());
    submitWorld();
    m_renderer.drawScene();
    m_particles.render(m_renderer);

    m_flash = std::max(0.0f, m_flash - dt * 2.4f);
    m_renderer.endFrame(m_camera.speedBlur(), m_flash);

    drawInterface(dt);
    m_window.swapBuffers();
}

void Game::drawInterface(float dt) {
    Input& in = m_window.input();
    m_ui.begin(m_window.framebufferWidth(), m_window.framebufferHeight());
    m_menu.tick(dt);

    // Handle splash timing when entering Loading
    if (m_state != m_prevState) {
        if (m_state == GameState::Loading) {
            m_splashTimer = 0.0f;
            m_splashAlpha = 0.0f;
            m_splashDone  = false;
        }
    }

    // Draw optional splash background if present during Loading
    if (m_state == GameState::Loading && m_resources.splash().valid() && !m_splashDone) {
        // use already-declared `in` above
        // durations (seconds)
        const float kFadeIn = 0.6f;
        const float kHold   = 1.2f;
        const float kFadeOut = 0.6f;
        const float kTotal  = kFadeIn + kHold + kFadeOut;

        // Advance timer
        m_splashTimer += dt;

        // Allow skipping by common inputs
        const bool skip = in.keyPressed(GLFW_KEY_ENTER) || in.keyPressed(GLFW_KEY_SPACE) ||
                          in.keyPressed(GLFW_KEY_ESCAPE) || in.mousePressed(GLFW_MOUSE_BUTTON_LEFT) ||
                          in.keyPressed(GLFW_KEY_W) || in.keyPressed(GLFW_KEY_S) ||
                          in.keyPressed(GLFW_KEY_UP) || in.keyPressed(GLFW_KEY_DOWN) ||
                          in.keyPressed(GLFW_KEY_LEFT) || in.keyPressed(GLFW_KEY_RIGHT);
        if (skip && m_splashTimer < (kFadeIn + kHold)) {
            m_splashTimer = kFadeIn + kHold; // jump to fade-out
        }

        // Compute alpha
        if (m_splashTimer < kFadeIn) {
            m_splashAlpha = m_splashTimer / kFadeIn;
        } else if (m_splashTimer < kFadeIn + kHold) {
            m_splashAlpha = 1.0f;
        } else if (m_splashTimer < kTotal) {
            m_splashAlpha = 1.0f - (m_splashTimer - (kFadeIn + kHold)) / kFadeOut;
        } else {
            m_splashAlpha = 0.0f;
            m_splashDone = true;
        }

        const int fbw = m_window.framebufferWidth();
        const int fbh = m_window.framebufferHeight();
        m_ui.texturedRect(m_resources.splash(), 0.0f, 0.0f, (float)fbw, (float)fbh, glm::vec3(1.0f), m_splashAlpha);
    }

    MenuAction action = MenuAction::None;
    switch (m_state) {
        case GameState::Loading:
            m_menu.drawLoading(m_ui, m_loadProgress);
            break;
        case GameState::MainMenu:
            action = m_menu.drawMainMenu(m_ui, in);
            break;
        case GameState::ColorSelect:
            action = m_menu.drawColorSelect(m_ui, in, m_settings);
            break;
        case GameState::SettingsMenu:
            action = m_menu.drawSettings(m_ui, in, m_settings);
            break;
        case GameState::Race:
            if (m_worldReady) {
                m_hud.draw(m_ui, m_cars[0], m_race, m_track, m_cars, m_fps, m_settings.showFps,
                           dt);
            }
            break;
        case GameState::Paused:
            if (m_worldReady) {
                m_hud.draw(m_ui, m_cars[0], m_race, m_track, m_cars, m_fps, m_settings.showFps,
                           dt);
            }
            action = m_menu.drawPause(m_ui, in);
            break;
        case GameState::Results:
            action = m_menu.drawResults(m_ui, in, m_race, m_cars);
            break;
    }
    m_ui.end();

    // remember state for next frame
    m_prevState = m_state;

    // ---- act on the menu result
    switch (action) {
        case MenuAction::GoColorSelect:
            m_state = GameState::ColorSelect;
            m_menu.resetFocus();
            m_lastAppliedColor = -1;  // force livery regen on first color select frame
            break;
        case MenuAction::GoSettings:
            m_returnState = m_state;
            m_state       = GameState::SettingsMenu;
            m_menu.resetFocus();
            break;
        case MenuAction::StartRace:
            m_settings.save();
            startRace();
            break;
        case MenuAction::GoMainMenu:
            m_state = GameState::MainMenu;
            m_menu.resetFocus();
            m_window.setCursorVisible(true);
            m_audio.setMusic(MusicMood::Menu);
            for (int i = 0; i < kCarCount; ++i) m_cars[(size_t)i].reset(m_track, i);
            break;
        case MenuAction::Resume:
            m_state = GameState::Race;
            m_window.setCursorVisible(false);
            m_audio.setMusic(MusicMood::Race);
            break;
        case MenuAction::Restart:
            startRace();
            break;
        case MenuAction::Back:
            m_settings.save();
            m_state = (m_returnState == GameState::SettingsMenu) ? GameState::MainMenu
                                                                 : m_returnState;
            m_menu.resetFocus();
            break;
        case MenuAction::ApplyVideo:
            applyVideoSettings();
            break;
        case MenuAction::Exit:
            m_running = false;
            break;
        case MenuAction::None:
        default:
            break;
    }
}

// ========================================================================= loop
void Game::run() {
    double last = m_window.time();
    float  fpsAccum = 0.0f;
    int    fpsFrames = 0;

    while (m_running && !m_window.shouldClose()) {
        const double now = m_window.time();
        float dt = (float)(now - last);
        last = now;
        dt = math::clampf(dt, 0.0f, 0.05f);
        m_time += dt;

        fpsAccum += dt;
        ++fpsFrames;
        if (fpsAccum > 0.35f) {
            m_fps     = (float)fpsFrames / fpsAccum;
            fpsAccum  = 0.0f;
            fpsFrames = 0;
        }

        m_window.pollEvents();
        handleGlobalKeys();

        switch (m_state) {
            case GameState::Loading:
                stepLoading();
                break;
            case GameState::MainMenu:
            case GameState::SettingsMenu:
                updateMenuCamera(dt);
                m_audio.setEngine(950.0f, 0.0f, 0.0f);
                m_audio.setTyreSlip(0.0f);
                break;
            case GameState::ColorSelect:
                updateMenuCamera(dt);
                m_audio.setEngine(950.0f, 0.0f, 0.0f);
                m_audio.setTyreSlip(0.0f);
                // Live-update the player car livery when a new colour is picked.
                if (m_lastAppliedColor != m_settings.carColor) {
                    m_lastAppliedColor = m_settings.carColor;
                    const glm::vec3 c(kCarColors[m_settings.carColor].r,
                                      kCarColors[m_settings.carColor].g,
                                      kCarColors[m_settings.carColor].b);
                    m_resources.regeneratePlayerLivery(c, 12);
                    m_cars[0].configure(c, false, 0, kDriverNames[0]);
                }
                break;
            case GameState::Race:
                updateRace(dt);
                break;
            case GameState::Paused:
                m_audio.setEngine(950.0f, 0.0f, 0.0f);
                m_audio.setTyreSlip(0.0f);
                break;
            case GameState::Results:
                m_particles.update(dt);
                updateMenuCamera(dt);
                break;
        }

        renderFrame(dt);
    }
}

void Game::shutdown() {
    m_settings.save();
    m_audio.shutdown();
    m_particles.shutdown();
    m_ui.shutdown();
    m_renderer.shutdown();
    m_window.destroy();
}

} // namespace vr
