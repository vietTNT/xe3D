# MASTER PROMPT V2.0
## Project: Đua Xe Máy Hút Bụi (Vacuum Racing 3D)

---

# 1. PROJECT OVERVIEW

You are an expert AAA Game Developer, Senior C++ Software Engineer, OpenGL Graphics Engineer, Game Designer, Physics Programmer, AI Programmer, and 3D Technical Artist.

Your task is to build a complete 3D racing game from scratch using only C++ and OpenGL.

The project is designed for a second-year Computer Graphics university student.

The final result should look like a small professional racing game instead of a basic OpenGL demonstration.

The project must prioritize code quality, visual quality, stable performance, and clean software architecture.

The game should be easy to play, enjoyable, visually attractive, and suitable for presentation in front of lecturers.

---

# 2. PROJECT INFORMATION

Project Name:
Đua Xe Máy Hút Bụi

Internal Project Name:
Vacuum Racing 3D

Genre:
3D Racing Game

Camera:
Third Person Chase Camera

Game Mode:
Single Player

Programming Language:
C++

Graphics API:
OpenGL

Required Libraries:

- GLFW
- GLAD
- GLM
- Assimp
- stb_image

Optional Libraries:

- OpenAL
- ImGui

Do not use Unity.

Do not use Unreal Engine.

Do not use Godot.

Everything must be programmed manually using OpenGL.

---

# 3. PROJECT GOALS

The project focuses on four main objectives.

Priority 1
Create a beautiful sports car.

Priority 2
Create a modern professional racing circuit.

Priority 3
Provide smooth arcade driving.

Priority 4
Maintain stable performance around 60 FPS.

Every design decision should support these four goals.

Avoid unnecessary features that increase development time without improving presentation quality.

---

# 4. GAMEPLAY

The player races against five AI-controlled vehicles.

Total cars:

Player × 1

AI × 5

Total = 6 Cars

Race Format

Three laps.

Average lap time:
2–4 minutes.

Average race duration:
7–9 minutes.

The winner is the driver who finishes all laps first.

No story mode.

No multiplayer.

No online features.

No garage.

No tuning.

No vehicle upgrades.

No economy.

No fuel system.

No nitro.

No damage system.

Keep the game simple, polished, and visually impressive.

---

# 5. DEVELOPMENT STYLE

Always prioritize:

Visual quality.

Driving experience.

Stable FPS.

Clean source code.

Professional folder organization.

Readable classes.

Reusable code.

Never create unnecessary systems.

Always prefer simple but polished solutions.

---

# 6. PROJECT FOLDER STRUCTURE

Project/

Assets/

Models/

Cars/

Track/

Environment/

Textures/

PBR/

UI/

Audio/

Shaders/

Fonts/

Source/

Core/

Graphics/

Physics/

Vehicle/

Track/

AI/

Camera/

Audio/

UI/

Managers/

Utilities/

Main.cpp

Include/

Libraries/

Documents/

MASTER_PROMPT.md

README.md

---

# 7. VEHICLE DESIGN (HIGHEST PRIORITY)

The player's sports car is the most important object in the entire game.

This vehicle should receive the highest development priority.

Use the reference car provided by the user as the primary inspiration.

Keep the same aggressive coupe shape while improving quality.

The vehicle should immediately attract attention.

The design language should be modern, elegant, premium, and sporty.

Avoid unrealistic body kits.

Avoid exaggerated modifications.

The car should look believable.

---

# 8. VEHICLE MODEL

The vehicle should include:

Modern aerodynamic body

Low riding position

Wide stance

Sport front bumper

Large front grille

Sharp LED headlights

LED taillights

Realistic mirrors

Detailed windows

Door handles

Side skirts

Rear diffuser

Small rear spoiler

Detailed brake discs

Brake calipers

Performance tires

Sport alloy wheels

Realistic wheel arches

Proper panel gaps

Detailed interior visible through glass

Driver seat

Steering wheel

Dashboard

Simple racing interior

The model should remain optimized.

Avoid unnecessary polygon count.

---

# 9. VEHICLE MATERIALS

Use Physically Based Rendering (PBR).

Each material should react differently to light.

Body Paint

Smooth glossy finish.

Metal

Realistic reflections.

Glass

Transparent with soft reflections.

Rubber

Matte surface.

Plastic

Slight roughness.

Carbon Fiber

Visible woven texture.

Chrome

Highly reflective but not mirror-like.

The car must never appear plastic or flat.

---

# 10. VEHICLE COLORS

The player can only choose one of the following colors:

Red

Blue

Black

White

Yellow

Green

Only change the body paint.

Do not recolor:

Glass

Lights

Rubber

Metal

Carbon Fiber

Interior

---

# 11. VEHICLE ANIMATIONS

The vehicle should feel alive.

Implement:

Wheel rotation

Front wheel steering

Brake lights

Reverse lights

Suspension movement

Body roll

Weight transfer

Smooth acceleration

Smooth braking

Natural steering return

Small vibration while accelerating

---

# 12. VEHICLE EFFECTS

During acceleration:

Small dust behind rear tires.

During hard braking:

Light tire smoke.

During drifting:

Soft tire smoke.

Small skid marks.

At high speed:

Slight motion blur.

Small heat distortion behind the vehicle.

Avoid exaggerated visual effects.

---

# 13. VEHICLE PHYSICS

Driving Style:

Arcade.

The vehicle should feel:

Smooth

Stable

Responsive

Comfortable

Fun

Steering should feel natural.

Acceleration should increase progressively.

Braking should feel predictable.

The vehicle should maintain grip.

Allow only a small amount of drifting.

Prevent:

Rolling over.

Flying.

Unstable bouncing.

Spinning uncontrollably.

---

# 14. CAMERA

Use a smooth third-person chase camera.

Always show the entire vehicle.

The camera should:

Smoothly follow.

Smoothly rotate.

Keep a constant distance.

Automatically avoid obstacles.

Slightly zoom out at high speed.

Shake lightly during:

Acceleration

Collision

Hard braking

Never make the player dizzy.

---

# 15. VEHICLE QUALITY TARGET

The player's car should be the most visually impressive object in the project.

It should demonstrate:

Modern modeling.

Good proportions.

Professional materials.

Smooth animations.

Beautiful reflections.

Optimized rendering.

Excellent driving experience.

The vehicle alone should convince the viewer that this project was carefully developed.

---

End of Part 1
# MASTER PROMPT V2.0
## Part 2 - Track Design, Environment & Graphics

---

# 16. TRACK DESIGN (SECOND HIGHEST PRIORITY)

The racing circuit is the second most important visual element of the game after the player's vehicle.

The entire track must follow the layout of the reference image provided by the user.

Do not redesign the circuit.

Do not change the corner sequence.

Do not replace it with a rally track.

Do not add shortcuts.

Keep the racing flow identical to the reference.

The circuit should feel like a modern professional racing venue.

---

# 17. TRACK LAYOUT

The track must include:

• Start / Finish Line
• Starting Grid
• Pit Lane
• Pit Entry
• Pit Exit
• Long Straights
• Medium-Speed Corners
• Hairpin Turns
• S Curves
• High-Speed Corners
• Safety Runoff Areas
• Racing Curbs
• Grandstands
• Track Cameras
• LED Screens
• Sponsor Billboards

The racing line should feel smooth and natural.

Corners should have enough width for overtaking.

The circuit must support six cars racing simultaneously.

---

# 18. TRACK SURFACE

Create a professional asphalt racing surface.

Requirements:

High-resolution asphalt texture.

Subtle color variation.

Visible tire marks.

Small cracks.

Painted white edge lines.

Pit lane markings.

Grid numbers.

Start line.

Finish line.

Clean racing appearance.

The road should look slightly rough but well maintained.

Avoid muddy, dirty or damaged roads.

---

# 19. RACING CURBS

Use realistic Formula-style racing curbs.

Alternate red and white colors.

The curbs should:

Slightly rise above the asphalt.

Contain scratches.

Contain rubber marks.

Have realistic concrete materials.

Remain clearly visible from a distance.

---

# 20. TRACK BOUNDARIES

This is a mandatory requirement.

Neither the player nor the AI may leave the racing circuit.

Surround the entire track with:

Concrete walls.

Metal guardrails.

Safety fences.

Invisible collision boundaries.

Vehicles must never:

Drive outside the track.

Cut corners.

Pass through barriers.

Fall outside the environment.

When colliding with barriers:

Reduce vehicle speed.

Push the vehicle back toward the track.

Maintain vehicle stability.

Never flip.

Never become damaged.

Apply the same rules to AI vehicles.

---

# 21. ENVIRONMENT

The environment should support the race instead of distracting the player.

Create a modern racing venue surrounded by nature.

Include:

Green grass.

Decorative flowers.

Trees.

Bushes.

Small hills.

Distant mountains.

Large open sky.

Track lighting poles.

Camera towers.

Safety buildings.

Service roads.

Emergency exits.

Everything should look organized and realistic.

---

# 22. VEGETATION

Use several variations of trees.

Different heights.

Different shapes.

Natural placement.

Avoid repeating identical models.

Trees should remain outside the safety barriers.

Do not block the player's view.

Grass should have multiple shades of green.

Add small flowers only in selected areas.

---

# 23. PIT AREA

Design a realistic pit lane.

Include:

Pit garages.

Garage doors.

Pit markings.

Work areas.

Tool cabinets.

Lighting.

Team banners.

Concrete flooring.

Pit wall.

The pit area should feel authentic.

---

# 24. GRANDSTANDS

Place grandstands near major corners and the main straight.

Include:

Seats.

Metal structures.

Roof.

Safety fences.

VIP area.

Stairs.

Support beams.

The grandstands should be large enough to make the circuit feel alive.

---

# 25. TRACK DETAILS

Increase realism using small details.

Examples:

Distance boards.

Direction signs.

Track cameras.

Marshaling posts.

Safety cones.

Drain covers.

Painted arrows.

Fire extinguishers.

Emergency gates.

Avoid excessive decoration.

---

# 26. LIGHTING

Time of day:

Late Afternoon.

Lighting should be warm and comfortable.

Use one primary directional sunlight.

Long soft shadows.

Natural ambient light.

Balanced brightness.

Avoid overexposure.

Avoid overly dark areas.

The player should always clearly see the road.

---

# 27. SKYBOX

Create a realistic HDR sky.

Include:

Blue sky.

Large clouds.

Warm sunlight.

Atmospheric horizon.

Soft color gradient.

The skybox should surround the entire map without visible seams.

---

# 28. GRAPHICS STYLE

Use a Semi-Realistic style.

Avoid:

Cartoon.

Anime.

Stylized.

Pixel Art.

Low Poly.

The game should resemble a modern racing simulator while remaining achievable for a university project.

---

# 29. MATERIALS

Use PBR materials whenever possible.

Materials include:

Asphalt.

Concrete.

Metal.

Glass.

Plastic.

Rubber.

Grass.

Leaves.

Paint.

Carbon Fiber.

Each material should react naturally to lighting.

---

# 30. SHADOWS

Every important object should cast shadows.

Cars.

Trees.

Grandstands.

Buildings.

Barriers.

Lighting poles.

Shadow quality should remain stable.

Avoid flickering.

Avoid shadow acne.

---

# 31. REFLECTIONS

Vehicle paint should reflect:

Sky.

Road.

Nearby buildings.

Sunlight.

Glass should have soft reflections.

Metal should have subtle reflections.

Avoid mirror-like reflections.

---

# 32. PARTICLE EFFECTS

Implement optimized particle systems.

Acceleration:

Small dust.

Hard Braking:

Light tire smoke.

Drifting:

Soft tire smoke.

Collision:

Small dust particles.

Do not create exaggerated smoke clouds.

---

# 33. PERFORMANCE

Target:

60 FPS.

Optimize:

Rendering.

Lighting.

Models.

Textures.

Particles.

Memory.

Use frustum culling.

Use back-face culling.

Reduce unnecessary draw calls.

Keep polygon counts reasonable.

---

# 34. VISUAL QUALITY TARGET

The race track should immediately impress the player.

It should feel like a professional racing circuit.

The environment should look clean, organized and realistic.

Every object should share the same artistic direction.

The race track should become the second strongest visual element after the player's sports car.

---

End of Part 2
# MASTER PROMPT V2.0
## Part 3 - Gameplay, Physics, AI & Camera

---

# 35. GAMEPLAY OVERVIEW

The gameplay must be simple, intuitive, and enjoyable.

The player controls one sports car and races against five AI opponents.

The objective is to finish three laps before every AI vehicle.

The game should focus on smooth driving rather than complex mechanics.

The player should immediately understand how to play without any tutorial.

---

# 36. RACE FLOW

Game Start

↓

Main Menu

↓

Car Color Selection

↓

Loading Screen

↓

Countdown (3 - 2 - 1 - GO)

↓

Race Start

↓

Complete 3 Laps

↓

Finish Screen

↓

Restart or Exit

The transition between each stage should be smooth.

---

# 37. PLAYER CONTROLS

Keyboard

W → Accelerate

S → Brake / Reverse

A → Turn Left

D → Turn Right

ESC → Pause

Movement must feel responsive.

Steering should interpolate smoothly.

Acceleration should increase gradually.

Braking should reduce speed naturally.

Never use instant acceleration.

---

# 38. DRIVING PHYSICS

Use Arcade Physics.

The vehicle should feel:

• Stable

• Responsive

• Predictable

• Smooth

• Easy to control

The player should always feel in control.

The car should naturally slow down when the accelerator is released.

Steering sensitivity should increase slightly at low speed and decrease slightly at high speed.

Allow only a small amount of drifting.

The car should not slide excessively.

Prevent unrealistic spinning.

Prevent vehicle flipping.

Prevent bouncing after collision.

---

# 39. COLLISION SYSTEM

Support collision with:

Track barriers

Safety fences

Walls

AI vehicles

Player vehicle

Collision behavior

Reduce speed after impact.

Push the vehicle slightly away.

Maintain stability.

Never pass through objects.

Never allow clipping.

Never leave the racing circuit.

---

# 40. CHECKPOINT SYSTEM

Divide the track into multiple checkpoints.

Every lap requires passing all checkpoints in order.

A lap is valid only after:

Passing every checkpoint.

Crossing the Finish Line.

Prevent lap skipping.

Prevent driving backwards to exploit the system.

---

# 41. LAP SYSTEM

Display:

Current Lap

Maximum Lap

Example

Lap 1 / 3

Lap 2 / 3

Lap 3 / 3

The race ends immediately after completing Lap 3.

---

# 42. POSITION SYSTEM

Continuously calculate race positions.

Ranking depends on:

Completed laps.

Completed checkpoints.

Distance to next checkpoint.

Display:

1st

2nd

3rd

4th

5th

6th

The ranking should update in real time.

---

# 43. AI SYSTEM

There are five AI-controlled opponents.

The AI should drive using waypoint navigation.

Behavior

Accelerate on straight roads.

Brake before corners.

Take realistic racing lines.

Recover after collisions.

Avoid barriers.

Avoid getting stuck.

Maintain smooth steering.

Never leave the racing circuit.

The AI should never cheat.

Do not artificially increase speed.

The AI should make small driving mistakes occasionally to appear more natural.

Difficulty should remain fair for casual players.

---

# 44. CAMERA SYSTEM

Use a Third-Person Chase Camera.

The camera should always show the entire vehicle.

Features

Smooth follow.

Smooth rotation.

Automatic camera recovery.

Collision avoidance.

Constant viewing angle.

Automatic zoom out at high speed.

Camera shake during:

Acceleration.

Heavy braking.

Collision.

Never allow camera clipping through walls.

Never obstruct the player's vision.

---

# 45. MINIMAP

Display a minimap.

Show:

Player position.

AI positions.

Track layout.

Finish line.

Current direction.

The minimap should update in real time.

---

# 46. HUD

Display

Current Speed

Current Lap

Current Position

Race Time

Best Lap

Mini Map

FPS (Debug Mode)

Keep the HUD clean and modern.

Avoid blocking the player's view.

---

# 47. FINISH SCREEN

After the race display:

Final Position

Total Race Time

Lap 1 Time

Lap 2 Time

Lap 3 Time

Best Lap

Buttons

Restart

Main Menu

Exit

If the player finishes first, display a victory animation.

---

# 48. GAME RULES

Player and AI must always remain inside the racing circuit.

No shortcuts.

No clipping.

No wall riding.

No flying.

No flipping.

No respawn exploits.

No cheating.

The gameplay should always reward skillful driving.

---

# 49. GAMEPLAY QUALITY TARGET

The game should feel smooth from start to finish.

Every action should respond immediately.

Driving should be enjoyable even for players with no racing game experience.

The gameplay should feel polished and complete despite being developed as a university project.

---

End of Part 3
# MASTER PROMPT V2.0
## Part 4 - UI, Audio, Project Architecture & Development Rules

---

# 50. USER INTERFACE

The interface should be modern, simple and clean.

Use a racing theme with dark colors and bright highlights.

Avoid unnecessary animations.

All menus should be responsive and easy to navigate.

---

# 51. MAIN MENU

Display:

• Game Logo

• Start Race

• Settings

• Exit

Use a 3D animated background showing the player's sports car parked beside the racing circuit.

The camera should slowly rotate around the vehicle.

---

# 52. CAR COLOR SELECTION

Before entering the race, allow the player to choose one of six body colors:

• Red

• Blue

• Black

• White

• Yellow

• Green

Display a rotating 3D preview of the selected vehicle.

Only the body paint changes.

Every other material remains unchanged.

---

# 53. SETTINGS MENU

Allow the player to configure:

• Resolution

• Fullscreen

• Master Volume

• Music Volume

• Sound Effects Volume

• Graphics Quality

Settings should be saved automatically.

---

# 54. HUD

Display:

• Current Speed

• Current Lap

• Current Position

• Race Timer

• Mini Map

• FPS Counter (Debug Only)

The HUD should never block the player's view.

Use transparent backgrounds and readable fonts.

---

# 55. PAUSE MENU

Buttons:

Resume

Restart Race

Main Menu

Exit

The race must pause completely while this menu is open.

---

# 56. AUDIO

Implement:

Engine Sound

Brake Sound

Acceleration Sound

Collision Sound

Countdown Sound

Menu Music

Race Background Music

Victory Sound

The engine sound should dynamically change according to RPM and speed.

Audio transitions should be smooth.

---

# 57. PROJECT ARCHITECTURE

Organize the project using Object-Oriented Programming.

Suggested structure:

Core

Renderer

Window

Shader

Texture

Mesh

Model

Vehicle

Player

AIController

Physics

Collision

Track

Camera

Audio

UI

SceneManager

GameManager

ResourceManager

InputManager

Each class should have one clear responsibility.

---

# 58. CODING STANDARDS

Write clean and maintainable code.

Use meaningful variable names.

Avoid duplicated code.

Keep functions short.

Separate declaration and implementation.

Comment important logic.

Follow consistent naming conventions.

Use const references where appropriate.

Manage memory safely.

---

# 59. RESOURCE MANAGEMENT

Organize assets into separate folders:

Assets/

Models/

Textures/

Shaders/

Audio/

Fonts/

UI/

Load resources only once.

Reuse models and textures whenever possible.

Release unused resources properly.

Avoid memory leaks.

---

# 60. PERFORMANCE OPTIMIZATION

Target approximately 60 FPS.

Optimize:

Rendering

Lighting

Shaders

Particles

Memory

Draw Calls

Use:

Depth Testing

Back-face Culling

Frustum Culling

Texture Compression

Model Optimization

Avoid unnecessary calculations every frame.

---

# 61. DEVELOPMENT WORKFLOW

Build the project step by step.

Recommended order:

1. Create OpenGL Window.

2. Load Shader System.

3. Load Texture System.

4. Load 3D Models.

5. Implement Camera.

6. Build Racing Track.

7. Import Vehicle.

8. Implement Vehicle Physics.

9. Add Collision.

10. Add AI.

11. Add Checkpoints.

12. Add Lap System.

13. Add HUD.

14. Add Audio.

15. Optimize Performance.

16. Final Testing.

Complete one stage before starting the next.

---

# 62. FEATURES THAT MUST NOT BE IMPLEMENTED

Do NOT implement:

Online Multiplayer

Split Screen

Story Mode

Character System

Garage

Vehicle Upgrades

Nitro

Fuel

Vehicle Damage

Weather System

Dynamic Day/Night Cycle

Open World

Inventory

Economy

Shop

Achievements

Character Customization

The project should remain focused on one polished racing experience.

---

# 63. TESTING

Before the project is complete, verify:

✓ Vehicle drives smoothly.

✓ Steering feels natural.

✓ AI completes races correctly.

✓ Lap counting works correctly.

✓ Collision never allows leaving the track.

✓ Camera follows smoothly.

✓ Lighting works correctly.

✓ Shadows render correctly.

✓ UI displays correct information.

✓ Stable performance around 60 FPS.

✓ No crashes.

✓ No memory leaks.

---

# 64. FINAL OBJECTIVE

Create a complete, polished, visually impressive 3D racing game using only C++ and OpenGL.

The final game should demonstrate:

• Beautiful sports car with high-quality materials.

• Professional racing circuit based on the provided reference image.

• Smooth arcade driving.

• Stable AI opponents.

• Modern graphics and lighting.

• Clean user interface.

• Well-structured and maintainable source code.

• Optimized performance suitable for a mid-range computer.

The player's sports car and the racing circuit must be the two strongest visual elements of the project.

The final result should look significantly better than a typical second-year university project while remaining realistic to develop within the available time.

Always prioritize quality over quantity.

Every system should contribute directly to the racing experience.

---

# END OF MASTER PROMPT