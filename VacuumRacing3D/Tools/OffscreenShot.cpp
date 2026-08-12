// -----------------------------------------------------------------------------
//  OffscreenShot.cpp - renders the game to a PNG-able buffer without a window.
//
//  Creates an OpenGL 3.3 core context through EGL (works with Mesa's software
//  rasteriser), builds the whole world exactly like the game does, drives the
//  cars for a few seconds and writes frames as binary PPM files.
//
//  This is a development / CI tool: it proves the shaders compile and the scene
//  renders on a machine with no display. It is not part of the game.
//
//  Build (Linux, needs EGL):
//     g++ -std=c++17 Tools/OffscreenShot.cpp Source/**/*.cpp -ISource -lEGL
// -----------------------------------------------------------------------------
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "AI/AIController.h"
#include "Camera/ChaseCamera.h"
#include "Core/GL.h"
#include "Core/Paths.h"
#include "Graphics/ParticleSystem.h"
#include "Graphics/Renderer.h"
#include "Managers/RaceManager.h"
#include "Managers/ResourceManager.h"
#include "Track/Environment.h"
#include "Track/Track.h"
#include "UI/HUD.h"
#include "UI/UIRenderer.h"
#include "Vehicle/CarModel.h"
#include "Vehicle/Vehicle.h"

namespace {

constexpr int kWidth  = 1280;
constexpr int kHeight = 720;

void writePPM(const char* path, const std::vector<unsigned char>& rgba, int w, int h) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; --y) {          // OpenGL origin is bottom-left
        for (int x = 0; x < w; ++x) {
            const unsigned char* p = &rgba[(size_t)(y * w + x) * 4];
            std::fwrite(p, 1, 3, f);
        }
    }
    std::fclose(f);
}

} // namespace

int main(int argc, char** argv) {
    using namespace vr;
    paths::initFromExecutable(argv && argv[0] ? argv[0] : nullptr);

    // ---------------------------------------------------------------- EGL setup
    // Prefer Mesa's surfaceless platform: it needs neither X11 nor a GPU.
    EGLDisplay display = EGL_NO_DISPLAY;
    auto getPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress(
        "eglGetPlatformDisplayEXT");
    if (getPlatformDisplay) {
        display = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
    }
    if (display == EGL_NO_DISPLAY) display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        std::fprintf(stderr, "no EGL display\n");
        return 1;
    }
    EGLint major = 0, minor = 0;
    if (!eglInitialize(display, &major, &minor)) {
        std::fprintf(stderr, "eglInitialize failed\n");
        return 1;
    }
    std::printf("EGL %d.%d - %s\n", major, minor, eglQueryString(display, EGL_VENDOR));

    // We render into our own framebuffer object, so no EGL config is needed:
    // EGL_KHR_no_config_context lets us create a context without one.
    EGLConfig config = (EGLConfig)0;   // EGL_NO_CONFIG_KHR
    {
        const EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};
        EGLint numConfigs = 0;
        EGLConfig picked;
        if (eglChooseConfig(display, attribs, &picked, 1, &numConfigs) && numConfigs > 0) {
            config = picked;
        } else {
            std::printf("no matching EGL config - using EGL_NO_CONFIG_KHR\n");
        }
    }

    // Surfaceless: everything is rendered into our own framebuffer object.
    EGLSurface surface = EGL_NO_SURFACE;

    eglBindAPI(EGL_OPENGL_API);
    const EGLint ctxAttribs[] = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 3,
                                 EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                 EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (context == EGL_NO_CONTEXT) {
        std::fprintf(stderr, "GL 3.3 core context creation failed\n");
        return 1;
    }
    if (!eglMakeCurrent(display, surface, surface, context)) {
        std::fprintf(stderr, "eglMakeCurrent failed (0x%04X)\n", eglGetError());
        return 1;
    }

    if (!loadOpenGL((GLProcLoader)eglGetProcAddress)) {
        std::fprintf(stderr, "%d GL entry points missing\n", missingOpenGLFunctions());
        return 1;
    }
    std::printf("GL renderer: %s\nGL version : %s\n", (const char*)glGetString(GL_RENDERER),
                (const char*)glGetString(GL_VERSION));

    // ------------------------------------------------------------- build world
    const int quality = 1;
    Renderer  renderer;
    if (!renderer.init(kWidth, kHeight, quality)) {
        std::fprintf(stderr, "renderer init failed (shader compilation?)\n");
        return 1;
    }
    std::printf("shaders compiled and linked OK\n");

    // Capture target: the renderer resolves the post pass straight into this FBO.
    GLuint captureFbo = 0, captureColor = 0, captureDepth = 0;
    glGenFramebuffers(1, &captureFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFbo);
    glGenTextures(1, &captureColor);
    glBindTexture(GL_TEXTURE_2D, captureColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kWidth, kHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, captureColor, 0);
    glGenRenderbuffers(1, &captureDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, captureDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kWidth, kHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "capture framebuffer incomplete\n");
        return 1;
    }
    renderer.outputFramebuffer = captureFbo;

    ResourceManager resources;
    resources.loadAll(quality);
    Track track;
    track.build(resources, quality);
    Environment environment;
    environment.build(track, resources, quality);
    CarModel carModel;
    carModel.build(resources, quality);
    ParticleSystem particles;
    particles.init(resources, quality);
    UIRenderer ui;
    ui.init();
    HUD hud;

    constexpr int kCars = 6;
    std::vector<Vehicle>      cars((size_t)kCars);
    std::vector<AIController> ai((size_t)kCars);
    std::vector<VehicleInput> inputs((size_t)kCars);
    const char* names[kCars] = {"PLAYER", "R. VOSS", "K. MORI", "A. SILVA", "T. NGUYEN",
                                "L. FISCHER"};
    const glm::vec3 colors[kCars] = {{0.72f, 0.045f, 0.05f}, {0.05f, 0.16f, 0.55f},
                                     {0.90f, 0.70f, 0.05f},  {0.05f, 0.42f, 0.20f},
                                     {0.70f, 0.72f, 0.75f},  {0.55f, 0.10f, 0.55f}};
    for (int i = 0; i < kCars; ++i) {
        cars[(size_t)i].configure(colors[i], i != 0, i, names[i]);
        cars[(size_t)i].reset(track, i);
        ai[(size_t)i].init(i, 0.9f, 555u);
    }
    RaceManager race;
    race.begin(track, cars, 3);

    ChaseCamera camera;
    camera.reset(cars[0]);

    DirectionalLight light;
    light.direction     = glm::normalize(glm::vec3(-0.46f, -0.40f, -0.32f));
    light.color         = glm::vec3(1.0f, 0.80f, 0.58f);
    light.intensity     = 3.25f;
    light.ambientSky    = glm::vec3(0.28f, 0.35f, 0.48f);
    light.ambientGround = glm::vec3(0.17f, 0.15f, 0.12f);
    renderer.fogColor   = glm::vec3(0.72f, 0.75f, 0.80f);
    renderer.fogDensity = 0.00072f;

    const float dt = 1.0f / 60.0f;
    const int   warmupSeconds = (argc > 1) ? std::atoi(argv[1]) : 14;
    const int   warmupFrames  = warmupSeconds * 60;

    for (int frame = 0; frame < warmupFrames; ++frame) {
        if (race.phase() != RacePhase::Countdown) {
            for (int i = 0; i < kCars; ++i) {
                inputs[(size_t)i] = ai[(size_t)i].update(dt, cars[(size_t)i], track, cars);
            }
            for (int i = 0; i < kCars; ++i) {
                cars[(size_t)i].update(dt, inputs[(size_t)i], track, &particles);
            }
            for (int a = 0; a < kCars; ++a) {
                for (int b = a + 1; b < kCars; ++b) {
                    Vehicle::resolvePair(cars[(size_t)a], cars[(size_t)b]);
                }
            }
            for (int i = 0; i < kCars; ++i) cars[(size_t)i].reseat(track);
        }
        race.update(dt, track, cars);
        particles.update(dt);
        camera.update(dt, cars[0], track);
    }
    std::printf("simulated %d s: player at %.0f m, %.0f km/h\n", warmupSeconds,
                cars[0].trackDistance(), cars[0].speedKmh());

    // ------------------------------------------------------------------ render
    std::vector<unsigned char> pixels((size_t)kWidth * kHeight * 4);
    const char* shotNames[4] = {"shot_chase.ppm", "shot_close.ppm", "shot_bumper.ppm",
                                "shot_showcase.ppm"};

    for (int shot = 0; shot < 4; ++shot) {
        if (shot < 3) {
            camera.setMode((CameraMode)shot);
            for (int i = 0; i < 30; ++i) camera.update(dt, cars[0], track);   // settle
        } else {
            // Beauty shot: the menu orbit camera around the player's car.
            for (int i = 0; i < 190; ++i) {
                camera.updateOrbit(dt, cars[0].position() + glm::vec3(0.0f, 0.35f, 0.0f), 7.6f,
                                   2.2f);
            }
        }

        renderer.timeSeconds = 12.0f;
        renderer.beginFrame(camera.view((float)kWidth / (float)kHeight), light,
                            cars[0].position());
        track.collect(renderer);
        environment.collect(renderer);
        track.collectStartLights(renderer, 0, true);
        for (const Vehicle& v : cars) carModel.collect(renderer, v.visualState());
        renderer.drawScene();
        particles.render(renderer);
        renderer.endFrame(camera.speedBlur(), 0.0f);

        if (shot < 3) {
            ui.begin(kWidth, kHeight);
            hud.draw(ui, cars[0], race, track, cars, 60.0f, true, dt);
            ui.end();
        }

        glFinish();
        glBindFramebuffer(GL_FRAMEBUFFER, captureFbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glReadPixels(0, 0, kWidth, kHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        writePPM(shotNames[shot], pixels, kWidth, kHeight);

        const RenderStats& st = renderer.stats();
        std::printf("%-18s submitted %4d  drawn %4d  triangles %7d\n", shotNames[shot],
                    st.submitted, st.drawn, st.triangles);
    }

    checkGLError("final");
    ui.shutdown();
    particles.shutdown();
    renderer.shutdown();
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
    std::printf("done\n");
    return 0;
}
