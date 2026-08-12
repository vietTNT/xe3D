# Architecture notes

## Layering

```
Main.cpp
  └── Game                     state machine, main loop, glue
        ├── Window/Input       GLFW, OpenGL 3.3 core context
        ├── Renderer           shadow pass → scene pass → post pass
        │     ├── Shader, Mesh, Texture, Material, Framebuffer
        │     └── ParticleSystem
        ├── World
        │     ├── Track        spline, geometry, queries, checkpoints, pit lane
        │     ├── Environment  grandstands, vegetation, signage, horizon
        │     └── CarModel     procedural sports car
        ├── Simulation
        │     ├── Vehicle      arcade physics, barrier + car collisions
        │     ├── AIController waypoint racing brain
        │     └── RaceManager  lights, laps, standings, results
        ├── ChaseCamera
        └── UI                 UIRenderer → HUD, MenuSystem
```

Each class owns one responsibility and never reaches upward: `Vehicle` knows
about `Track` but not about `Game`; `Track` knows about `Renderer` only to
submit draw calls.

## Frame

```
poll input
  ↓
update (substepped at ≤ 8.5 ms for physics stability)
    player input → Vehicle
    AI (60 Hz)   → Vehicle × 5
    pairwise car separation
    RaceManager: checkpoints, laps, standings
    particles, camera, audio telemetry
  ↓
render
    Renderer::beginFrame  – camera + light + frustum planes + shadow matrix
    submit                – track, scenery, 6 cars, start lights  (culled)
    drawScene             – shadow pass, sky, opaque, sorted transparent
    ParticleSystem::render– skid marks and billboards
    endFrame              – speed blur, FXAA, tonemap, vignette
    UI                    – HUD or menu
  ↓
swap
```

## Notable design decisions

**No GLAD.** `Source/Core/GL.h` declares the ~70 OpenGL entry points the engine
uses through a single X-macro list and resolves them with `glfwGetProcAddress`.
It keeps the dependency list at GLFW + GLM and makes the GL surface area of the
project explicit and auditable.

**Everything procedural.** Textures come from value/Worley noise
(`Utilities/Noise`), the font is a 1-bit bitmap compiled into
`Graphics/FontData.h`, audio is synthesised in the mixer callback. The result is
a repository with zero binary assets that always builds and always runs.

**The car body is a loft, not a box pile.** `kStations` in `CarModel.cpp`
defines 20 cross sections (half width, floor height, roof height, tumblehome).
Each section is a superellipse whose width shrinks toward the roof, and adjacent
sections are bridged with smooth normals. Every quad of the loft is then
classified — above the beltline and inside the cabin becomes glass, the top
centre becomes roof, the A/B/C pillar bands stay painted — which is what makes
the greenhouse read as a real structure.

**The circuit is data, not code.** The 152 control points in `CircuitData.h`
came from a layout of straights and arcs solved offline for exact closure. The
engine only samples the Catmull-Rom spline, computes frames, curvature and
banking, then extrudes geometry from those samples. Reshaping the track means
editing numbers.

**The racing line is computed, not authored.** Apex targets are derived from
curvature, smoothed 90 times into an out-in-out trajectory, and the resulting
line's own curvature gives a reference speed per sample. A backward pass then
lowers each speed so that the previous sample is always reachable under braking,
which is exactly the information the AI needs to brake at the right moment.

**Barriers are analytic.** Instead of colliding against wall meshes, the physics
projects the car onto the spline and clamps the lateral offset. A car therefore
cannot tunnel through a barrier at any speed or frame rate, which is what makes
the "never leave the circuit" requirement actually hold.

**Grip as velocity realignment.** Rather than a full tyre model, the velocity
vector is exponentially rotated toward the heading. A high rate feels planted, a
low rate slides; the handbrake and hard cornering lower it. This is stable at
any timestep and never spins the car uncontrollably.

## Performance

| Pass | Cost driver | Mitigation |
|---|---|---|
| Shadow | scene triangles | one fitted ortho frustum around the player, texel snapped, casters only |
| Scene | draw calls | scenery merged into ~17 meshes, track into ~13, frustum culling per call |
| Transparent | overdraw | glass only, sorted back to front, no depth write |
| Particles | fill rate | ≤ 1,400 billboards, ≤ 1,600 skid quads in a ring buffer |
| Post | full screen | single pass, FXAA and blur disabled on Low |

Typical frame: ~160 submitted draws, ~45 survive culling, ~290 k triangles.
