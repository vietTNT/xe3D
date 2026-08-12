// -----------------------------------------------------------------------------
//  HeadlessSim.cpp - runs a full 3 lap race with no window and no GPU.
//
//  Every OpenGL entry point is replaced by a stub, so the geometry builders and
//  the whole gameplay stack (track, physics, AI, checkpoints, lap timing,
//  standings) can be exercised in a terminal. Use it to regression test the
//  driving model after tuning, or on a machine without a graphics driver.
//
//  Build:  cmake -B build -DVR_BUILD_HEADLESS_SIM=ON && cmake --build build
//  Run  :  ./build/HeadlessSim
// -----------------------------------------------------------------------------
#include <cmath>
#include <cstdio>
#include <vector>

#include "AI/AIController.h"
#include "Core/GL.h"
#include "Managers/RaceManager.h"
#include "Managers/ResourceManager.h"
#include "Track/Track.h"
#include "Utilities/MathUtils.h"
#include "Vehicle/CarModel.h"
#include "Vehicle/Vehicle.h"

namespace {

/// Installs harmless implementations for every GL function the engine uses.
void installStubGL() {
#define X(ret, name, params)          \
    name = +[] params -> ret {        \
        using R = ret;                \
        return R();                   \
    };
    VR_GL_FUNCTIONS
#undef X

    // A few calls must return believable values for the engine to keep going.
    glGenBuffers      = [](GLsizei n, GLuint* out) { for (GLsizei i = 0; i < n; ++i) out[i] = 1; };
    glGenVertexArrays = [](GLsizei n, GLuint* out) { for (GLsizei i = 0; i < n; ++i) out[i] = 1; };
    glGenTextures     = [](GLsizei n, GLuint* out) { for (GLsizei i = 0; i < n; ++i) out[i] = 1; };
    glGenFramebuffers = [](GLsizei n, GLuint* out) { for (GLsizei i = 0; i < n; ++i) out[i] = 1; };
    glGenRenderbuffers = [](GLsizei n, GLuint* out) { for (GLsizei i = 0; i < n; ++i) out[i] = 1; };
    glCreateShader    = [](GLenum) -> GLuint { return 1; };
    glCreateProgram   = []() -> GLuint { return 1; };
    glGetShaderiv     = [](GLuint, GLenum, GLint* v) { *v = GL_TRUE; };
    glGetProgramiv    = [](GLuint, GLenum, GLint* v) { *v = GL_TRUE; };
    glGetUniformLocation = [](GLuint, const GLchar*) -> GLint { return -1; };
    glCheckFramebufferStatus = [](GLenum) -> GLenum { return GL_FRAMEBUFFER_COMPLETE; };
    glGetString       = [](GLenum) -> const GLubyte* { return (const GLubyte*)"headless"; };
    glGetError        = []() -> GLenum { return GL_NO_ERROR; };
}

bool finite3(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

std::string timeStr(float t) {
    char buf[24];
    vr::math::formatLapTime(t, buf, sizeof(buf));
    return std::string(buf);
}

} // namespace

int main() {
    using namespace vr;
    installStubGL();

    std::printf("=== Vacuum Racing 3D - headless race simulation ===\n");

    ResourceManager resources;
    resources.loadAll(0);

    Track track;
    if (!track.build(resources, 0)) {
        std::printf("FAILED: track build\n");
        return 1;
    }

    constexpr int kCars = 6;
    std::vector<Vehicle>      cars((size_t)kCars);
    std::vector<AIController> ai((size_t)kCars);
    std::vector<VehicleInput> inputs((size_t)kCars);

    const char* names[kCars] = {"PLAYER", "R. VOSS", "K. MORI",
                                "A. SILVA", "T. NGUYEN", "L. FISCHER"};
    for (int i = 0; i < kCars; ++i) {
        cars[(size_t)i].configure(glm::vec3(0.8f, 0.1f, 0.1f), i != 0, i, names[i]);
        cars[(size_t)i].reset(track, i);
        // The player car is driven by the AI too, so the race can complete.
        ai[(size_t)i].init(i, 0.86f + 0.022f * (float)i, 4242u);
    }

    // Dump the circuit so external tools can draw a map of it.
    if (FILE* csv = std::fopen("track_layout.csv", "w")) {
        std::fprintf(csv, "x,z,y,right_x,right_z,line_offset,curvature,line_speed\n");
        for (const TrackSample& s : track.samples()) {
            std::fprintf(csv, "%.3f,%.3f,%.3f,%.4f,%.4f,%.3f,%.6f,%.2f\n", s.position.x,
                         s.position.z, s.position.y, s.right.x, s.right.z, s.lineOffset,
                         s.curvature, s.lineSpeed);
        }
        std::fclose(csv);
        std::printf("[Sim] wrote track_layout.csv (%d samples)\n", track.sampleCount());
    }

    // The car model is needed to know where the wheels actually are, so the
    // "tyres stay on the tarmac" requirement can be measured rather than assumed.
    CarModel carModel;
    carModel.build(resources, 0);

    RaceManager race;
    race.begin(track, cars, 3);

    float maxWheelGap  = 0.0f;   // tyre floating above the road
    float maxWheelSink  = 0.0f;  // tyre buried under the road

    const float dt = 1.0f / 120.0f;
    float       elapsed = 0.0f;
    float       maxLateral = 0.0f;
    float       maxSpeed   = 0.0f;
    int         badFrames  = 0;
    int         aiTick     = 0;

    while (elapsed < 900.0f) {
        elapsed += dt;

        if (race.phase() != RacePhase::Countdown) {
            if (aiTick++ % 2 == 0) {
                for (int i = 0; i < kCars; ++i) {
                    inputs[(size_t)i] = ai[(size_t)i].update(dt * 2.0f, cars[(size_t)i], track,
                                                             cars);
                }
            }
            for (int i = 0; i < kCars; ++i) {
                cars[(size_t)i].update(dt, inputs[(size_t)i], track, nullptr);
            }
            for (int a = 0; a < kCars; ++a) {
                for (int b = a + 1; b < kCars; ++b) {
                    Vehicle::resolvePair(cars[(size_t)a], cars[(size_t)b]);
                }
            }
            for (int i = 0; i < kCars; ++i) cars[(size_t)i].reseat(track);
        }
        race.update(dt, track, cars);

        for (const Vehicle& v : cars) {
            if (!finite3(v.position()) || !finite3(v.velocity())) ++badFrames;
            maxLateral = std::max(maxLateral, std::fabs(v.lateralOffset()));
            maxSpeed   = std::max(maxSpeed, v.speed());
        }

        // ---- wheel contact check on the player's car
        {
            const CarVisualState st = cars[0].visualState();
            for (int wi = 0; wi < 4; ++wi) {
                glm::vec3 local = carModel.wheelOffsets()[wi];
                local.y -= st.suspension[wi];
                const glm::vec3 hub = glm::vec3(st.transform * glm::vec4(local, 1.0f));
                const float     tyreBottom = hub.y - CarModel::kWheelRadius;
                const float     ground = track.locate(hub, cars[0].trackHint()).surfaceY;
                const float     gap = tyreBottom - ground;
                if (gap > 0.0f) maxWheelGap = std::max(maxWheelGap, gap);
                else            maxWheelSink = std::max(maxWheelSink, -gap);
            }
        }
        if (race.phase() == RacePhase::Finished) break;
    }

    // ------------------------------------------------------------- report
    std::printf("\n--- classification after %s ---\n", timeStr(elapsed).c_str());
    std::printf("%-4s %-12s %-10s %-10s %-6s\n", "POS", "DRIVER", "TOTAL", "BEST LAP", "LAPS");
    for (const ResultRow& r : race.results()) {
        const Vehicle& v = cars[(size_t)r.carIndex];
        std::printf("%-4d %-12s %-10s %-10s %-6d\n", r.position, r.name.c_str(),
                    timeStr(r.totalTime).c_str(), timeStr(r.bestLap).c_str(),
                    std::max(v.race.lap, 0));
    }

    const float wallLimit = Track::kWallOffset;
    std::printf("\nlap length          : %.0f m\n", track.length());
    std::printf("max |lateral|       : %.2f m (barrier at %.2f m)\n", maxLateral, wallLimit);
    std::printf("max speed           : %.1f km/h\n", maxSpeed * 3.6f);
    std::printf("tyre above road     : %.1f mm (max)\n", maxWheelGap * 1000.0f);
    std::printf("tyre under road     : %.1f mm (max)\n", maxWheelSink * 1000.0f);
    std::printf("non-finite frames   : %d\n", badFrames);

    bool ok = true;
    if (badFrames != 0) { std::printf("FAIL: NaN/Inf detected\n"); ok = false; }
    if (maxLateral > wallLimit + 0.5f) { std::printf("FAIL: a car left the circuit\n"); ok = false; }
    if (race.phase() != RacePhase::Finished) { std::printf("FAIL: race never finished\n"); ok = false; }
    if (maxWheelSink > 0.025f) {
        std::printf("FAIL: a wheel sank %.0f mm below the road\n", maxWheelSink * 1000.0f);
        ok = false;
    }
    if (maxWheelGap > 0.040f) {
        std::printf("FAIL: a wheel floated %.0f mm above the road\n", maxWheelGap * 1000.0f);
        ok = false;
    }
    int finishers = 0;
    for (const Vehicle& v : cars) if (v.race.lap >= 3) ++finishers;
    if (finishers < 1) { std::printf("FAIL: nobody completed 3 laps\n"); ok = false; }

    std::printf("\n%s\n", ok ? "ALL CHECKS PASSED" : "CHECKS FAILED");
    return ok ? 0 : 1;
}
