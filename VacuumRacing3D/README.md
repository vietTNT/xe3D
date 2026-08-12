# Đua Xe Máy Hút Bụi — Vacuum Racing 3D

A complete 3D racing game written from scratch in **C++17 and OpenGL 3.3 Core**.
No game engine, no Unity, no Unreal, no Godot — every triangle, texture, shader
and sound is produced by this codebase.

| | |
|---|---|
| **Genre** | Single player 3D circuit racing |
| **Language / API** | C++17, OpenGL 3.3 Core |
| **Dependencies** | GLFW (window + input), GLM (maths), miniaudio (optional sound) |
| **Assets on disk** | none — geometry, textures, the font and the audio are all procedural |
| **Circuit** | 6,769 m, 14.4 m wide, 12 corners, 2 hairpins, an esse complex |
| **Race** | 6 cars (1 player + 5 AI), 3 laps, ~2:20 per lap, ~7 minutes total |

---

## 1. Build and run

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/VacuumRacing3D            # Windows: build\Release\VacuumRacing3D.exe
```

The first configure downloads GLFW, GLM and `miniaudio.h` automatically
(CMake `FetchContent`). Nothing else is required.

| CMake option | Default | Meaning |
|---|---|---|
| `VR_ENABLE_AUDIO` | ON | Procedural sound through miniaudio. `OFF` builds a silent game. |
| `VR_SYSTEM_GLFW` / `VR_SYSTEM_GLM` | OFF | Link an already installed GLFW/GLM instead of downloading. |
| `VR_BUILD_HEADLESS_SIM` | OFF | Also build `HeadlessSim`, a terminal race simulator (see §6). |
| `VR_BUILD_SMOKE_TEST` | OFF | Linux/EGL only. Drives the real game loop through every screen with no window. |
| `VR_WARNINGS` | ON | `-Wall -Wextra` / `/W4`. |

**Linux prerequisites** (only needed to build GLFW): `libx11-dev libxrandr-dev
libxinerama-dev libxcursor-dev libxi-dev`.
**Windows**: Visual Studio 2019+ works out of the box.
**macOS**: builds, but Apple caps OpenGL at 4.1 core — the game runs, some
drivers dislike the deprecated path.

---

## 2. Controls

| Key | Action |
|---|---|
| `W` / `↑` | Accelerate |
| `S` / `↓` | Brake, then reverse |
| `A` `D` / `←` `→` | Steer |
| `Space` | Handbrake (loosens the rear for a drift) |
| `C` | Cycle camera: chase → close → bumper |
| `R` | Restart the race |
| `Esc` | Pause / back |
| `F1` | FPS counter |
| Mouse + `W`/`S` + `Enter` | Menu navigation |

---

## 3. What is implemented

**Vehicle (top priority).** The car body is a *lofted surface*: 20 hand-authored
cross sections, each a tumblehome superellipse, swept into a smooth coupe. The
loft's quads are sorted at build time into paint / glass / roof / pillar groups,
so the greenhouse is real transparent glazing with A, B and C pillars rather than
a texture. On top of that: wheel arches with dark inner liners, a five twin-spoke
alloy wheel with brake disc and caliper, LED headlights and a full width tail
bar, carbon splitter, side skirts, diffuser and ducktail spoiler, mirrors, door
shut lines, handles, exhaust tips, and a modelled interior (seats, dashboard,
steering wheel, roll hoop) visible through the glass.

Six body colours — red, blue, black, white, yellow, green — repaint **only** the
paint mesh; glass, rubber, chrome, carbon and the interior never change.

**Rendering.** Forward renderer with Cook-Torrance PBR (GGX + Smith + Schlick),
a clear-coat lobe for the car paint, one directional late-afternoon sun, PCF
shadow mapping with texel snapping, an analytic sky that doubles as the
reflection probe, height fog, CPU frustum culling, then a post chain of radial
speed blur → FXAA → ACES tonemap → vignette.

**Circuit.** A closed Catmull-Rom spline through 152 control points (solved
offline so the loop closes to millimetre accuracy), resampled at 3 m. From it the
engine generates the asphalt with a rubbered-in racing line, painted edge lines,
red/white curbs on every corner, asphalt run-off, a grass apron that fades into
the ground plane, concrete walls, guard rails, debris fencing, the chequered
start line, painted grid boxes, a start light panel with five working lights and
a pit lane with garages and a pit wall. Nothing is built over the racing surface:
the circuit is completely open to the sky. Around it: five grandstands with stepped
seating, floodlight rigs, sponsor boards, two LED screens, marshal posts,
distance boards, cones, service buildings, ~500 trees in three varieties, flower
beds and a ring of hazy hills.

**Driving.** Arcade model: torque curve with speed fade, aerodynamic and rolling
drag, speed-sensitive steering, a bicycle-model yaw rate, and grip expressed as
how quickly the velocity vector realigns with the heading — high grip is
planted, low grip drifts.

The chassis rides on a **four wheel contact solver**: every frame the road height
is sampled under each wheel, a plane is fitted through the four contact points,
and the car is placed on that plane with no interpolation lag. Weight transfer
still rolls and pitches the body, but the suspension travel cancels that rotation
in closed form, so the tyres stay glued to the tarmac — measured at worst 14 mm
of sink and 28 mm of float across a full race, on a 710 mm tall tyre. The car **cannot** leave the circuit: the barrier test is
analytic against the spline's lateral limits, so no tunnelling, no flipping, no
falling out of the world.

**AI.** Five opponents drive the same `Vehicle` class with the same physics —
no rubber banding, no extra grip. Each follows the pre-computed racing line with
a speed-dependent look-ahead, scans 190 m forward and brakes exactly as early as
its own deceleration allows, avoids slower cars by offsetting its line,
counter-steers when the rear slides, occasionally makes a small mistake, and
un-sticks itself if it stops.

**Race.** Countdown with the five red lights, 24 ordered checkpoints per lap so
corners cannot be cut, lap validation, per-lap timing, best lap, live position
calculation and a final classification with gaps.

**Interface.** Speed, gear and RPM bar, lap counter, position, race and best-lap
times, live standings, a real-time minimap drawn from the spline, countdown,
banners, plus main menu, colour picker, settings (resolution, fullscreen,
v-sync, quality, three volumes — saved to `settings.cfg`), pause and results
screens. Text uses a 1-bit bitmap font compiled into the binary.

**Audio.** Synthesised live from telemetry: an engine built from four sawtooth
harmonics at the firing frequency with a throttle-driven low-pass, intake noise,
wind, resonant tyre squeal, impact bursts, countdown beeps, a victory fanfare
and a music bed.

---

## 4. Project layout

```
VacuumRacing3D/
├── CMakeLists.txt
├── Shaders/            scene, depth, sky, particle, ui, post  (GLSL 330)
├── Source/
│   ├── Core/           GL loader, window, input, path resolution
│   ├── Graphics/       shader, mesh + MeshBuilder, textures, materials,
│   │                   framebuffers, renderer, particles, font data
│   ├── Vehicle/        CarModel (procedural car), Vehicle (physics)
│   ├── Track/          Track (spline + geometry), Environment, CircuitData
│   ├── AI/             AIController
│   ├── Camera/         ChaseCamera
│   ├── UI/             UIRenderer, HUD, Menu
│   ├── Managers/       ResourceManager, RaceManager, Settings
│   ├── Audio/          procedural synthesiser
│   ├── Game.cpp/.h     state machine and main loop
│   └── Main.cpp
├── Tools/
│   ├── HeadlessSim.cpp     race simulation with no GPU
│   └── OffscreenShot.cpp   EGL screenshot tool (Linux)
└── Documents/          architecture notes, spec compliance, master prompt
```

---

## 5. Tuning it yourself

| I want to change… | Edit |
|---|---|
| The circuit shape | `Source/Track/CircuitData.h` — the 152 control points, or `kCircuitScale` |
| Track width, run-off, checkpoint count | the constants at the top of `Source/Track/Track.h` |
| How the car drives | `VehicleTuning` in `Source/Vehicle/Vehicle.h` |
| How the car looks | the `kStations` table in `Source/Vehicle/CarModel.cpp` |
| AI difficulty | the skill values passed to `AIController::init` in `Game::startRace` |
| Number of laps | `m_race.begin(m_track, m_cars, 3)` in `Game::startRace` |
| Time of day / sun | `m_light` in `Game::init` |

---

## 6. Testing without a GPU

```bash
cmake -B build -DVR_BUILD_HEADLESS_SIM=ON && cmake --build build -j
./build/HeadlessSim
```

Every OpenGL call is stubbed, so the geometry builders, physics, AI, checkpoints
and timing run in a terminal. It prints the full classification and asserts that
nothing produced a NaN, that no car left the circuit and that the race finished.
Handy as a regression test after tuning the handling.

On Linux with EGL available there is a second, complementary test:

```bash
cmake -B build -DVR_BUILD_SMOKE_TEST=ON && cmake --build build -j
./build/GameSmokeTest
```

It supplies its own GLFW entry points backed by an off-screen EGL context and
then runs the **real** `Game` loop, scripting key presses through loading, the
main menu, the colour picker, a race, a camera change, pause, restart, the
settings screen and exit. `HeadlessSim` never touches `Game.cpp`; this one does,
which is exactly where UI-order bugs hide.

---

## 7. Troubleshooting

* **"Could not locate the Shaders/ folder"** — run the executable from the
  project directory, or keep `Shaders/` next to the binary (CMake copies it
  there after every build).
* **Black screen / shader errors** — the log prints the exact GLSL error. A GPU
  or driver older than OpenGL 3.3 cannot run the game.
* **Low frame rate** — Settings → Graphics Quality → Low. That drops the shadow
  map to 1024², disables FXAA and speed blur, and thins out the scenery.
* **No sound** — build with `-DVR_ENABLE_AUDIO=ON` (needs the miniaudio
  download to have succeeded) and check the master volume in Settings.
