# =============================================================================
#  Dua Xe May Hut Bui / Vacuum Racing 3D
#  C++17 + OpenGL 3.3 core. Dependencies (GLFW, GLM and the optional miniaudio
#  header) are fetched automatically the first time you configure the project.
#
#  Quick start:
#     cmake -B build -DCMAKE_BUILD_TYPE=Release
#     cmake --build build -j
#     ./build/VacuumRacing3D          (Windows: build\Release\VacuumRacing3D.exe)
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(VacuumRacing3D VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(VR_ENABLE_AUDIO   "Procedural audio through miniaudio" ON)
option(VR_SYSTEM_GLFW    "Use an installed GLFW instead of downloading it" OFF)
option(VR_SYSTEM_GLM     "Use an installed GLM instead of downloading it"  OFF)
option(VR_WARNINGS       "Enable a strict warning set"                     ON)
option(VR_BUILD_HEADLESS_SIM "Build the terminal race simulator (no GPU needed)" OFF)
option(VR_BUILD_OFFSCREEN    "Build the EGL screenshot tool (Linux only)"        OFF)
option(VR_BUILD_SMOKE_TEST   "Build the Game state-machine smoke test (Linux/EGL)" OFF)

# -----------------------------------------------------------------------------
#  Dependencies
# -----------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ---- GLM (header only maths library)
if(VR_SYSTEM_GLM)
    find_package(glm REQUIRED)
else()
    FetchContent_Declare(glm
        URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
        DOWNLOAD_EXTRACT_TIMESTAMP ON)
    FetchContent_MakeAvailable(glm)
endif()

# ---- GLFW (window, context and input)
if(VR_SYSTEM_GLFW)
    find_package(glfw3 REQUIRED)
else()
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
        DOWNLOAD_EXTRACT_TIMESTAMP ON)
    FetchContent_MakeAvailable(glfw)
endif()

# ---- miniaudio (single header, only needed for sound)
set(VR_AUDIO_READY OFF)
if(VR_ENABLE_AUDIO)
    set(VR_MINIAUDIO_DIR "${CMAKE_BINARY_DIR}/_deps/miniaudio")
    set(VR_MINIAUDIO_HDR "${VR_MINIAUDIO_DIR}/miniaudio.h")
    if(NOT EXISTS "${VR_MINIAUDIO_HDR}")
        file(MAKE_DIRECTORY "${VR_MINIAUDIO_DIR}")
        message(STATUS "Downloading miniaudio.h ...")
        file(DOWNLOAD
            "https://raw.githubusercontent.com/mackron/miniaudio/0.11.21/miniaudio.h"
            "${VR_MINIAUDIO_HDR}"
            INACTIVITY_TIMEOUT 30
            STATUS VR_MA_STATUS)
        list(GET VR_MA_STATUS 0 VR_MA_CODE)
        if(NOT VR_MA_CODE EQUAL 0)
            file(REMOVE "${VR_MINIAUDIO_HDR}")
            message(WARNING "miniaudio download failed - building without audio.")
        endif()
    endif()
    if(EXISTS "${VR_MINIAUDIO_HDR}")
        set(VR_AUDIO_READY ON)
    endif()
endif()

# -----------------------------------------------------------------------------
#  Target
# -----------------------------------------------------------------------------
file(GLOB_RECURSE VR_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Source/*.cpp")
file(GLOB_RECURSE VR_HEADERS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Source/*.h")

add_executable(VacuumRacing3D ${VR_SOURCES} ${VR_HEADERS})

target_include_directories(VacuumRacing3D PRIVATE "${CMAKE_SOURCE_DIR}/Source")
target_link_libraries(VacuumRacing3D PRIVATE glfw)

if(TARGET glm::glm)
    target_link_libraries(VacuumRacing3D PRIVATE glm::glm)
elseif(DEFINED glm_SOURCE_DIR)
    target_include_directories(VacuumRacing3D PRIVATE "${glm_SOURCE_DIR}")
endif()

if(VR_AUDIO_READY)
    target_compile_definitions(VacuumRacing3D PRIVATE VR_ENABLE_AUDIO)
    target_include_directories(VacuumRacing3D PRIVATE "${VR_MINIAUDIO_DIR}")
    message(STATUS "Audio: procedural synthesis enabled")
else()
    message(STATUS "Audio: disabled (silent build)")
endif()

# The renderer resolves its own GL entry points, so no GL library is required.
find_package(Threads REQUIRED)
target_link_libraries(VacuumRacing3D PRIVATE Threads::Threads)
if(UNIX AND NOT APPLE)
    target_link_libraries(VacuumRacing3D PRIVATE ${CMAKE_DL_LIBS} m)
endif()

if(VR_WARNINGS)
    if(MSVC)
        target_compile_options(VacuumRacing3D PRIVATE /W4 /permissive-)
    else()
        target_compile_options(VacuumRacing3D PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()
endif()

if(MSVC)
    target_compile_definitions(VacuumRacing3D PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT VacuumRacing3D)
    set_target_properties(VacuumRacing3D PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# -----------------------------------------------------------------------------
#  Optional: headless race simulator (Tools/HeadlessSim.cpp)
#  Runs the full gameplay stack in a terminal with stubbed OpenGL calls.
# -----------------------------------------------------------------------------
if(VR_BUILD_HEADLESS_SIM)
    set(VR_SIM_SOURCES ${VR_SOURCES})
    list(REMOVE_ITEM VR_SIM_SOURCES "${CMAKE_SOURCE_DIR}/Source/Main.cpp")
    list(REMOVE_ITEM VR_SIM_SOURCES "${CMAKE_SOURCE_DIR}/Source/Game.cpp")
    list(REMOVE_ITEM VR_SIM_SOURCES "${CMAKE_SOURCE_DIR}/Source/Core/Window.cpp")
    add_executable(HeadlessSim "${CMAKE_SOURCE_DIR}/Tools/HeadlessSim.cpp" ${VR_SIM_SOURCES})
    target_include_directories(HeadlessSim PRIVATE "${CMAKE_SOURCE_DIR}/Source")
    if(TARGET glm::glm)
        target_link_libraries(HeadlessSim PRIVATE glm::glm)
    elseif(DEFINED glm_SOURCE_DIR)
        target_include_directories(HeadlessSim PRIVATE "${glm_SOURCE_DIR}")
    endif()
    # glfw is linked only for its key-code header; no window is ever created.
    target_link_libraries(HeadlessSim PRIVATE glfw Threads::Threads)
    if(UNIX AND NOT APPLE)
        target_link_libraries(HeadlessSim PRIVATE ${CMAKE_DL_LIBS} m)
    endif()
endif()

# -----------------------------------------------------------------------------
#  Optional: EGL offscreen screenshot tool (Tools/OffscreenShot.cpp)
#  Renders the game to PPM files without a window - useful for CI, for
#  verifying that the shaders compile, and for machines with no display.
# -----------------------------------------------------------------------------
if(VR_BUILD_OFFSCREEN AND UNIX AND NOT APPLE)
    find_library(VR_EGL_LIB EGL)
    if(VR_EGL_LIB)
        set(VR_SHOT_SOURCES ${VR_SOURCES})
        list(REMOVE_ITEM VR_SHOT_SOURCES "${CMAKE_SOURCE_DIR}/Source/Main.cpp")
        list(REMOVE_ITEM VR_SHOT_SOURCES "${CMAKE_SOURCE_DIR}/Source/Game.cpp")
        list(REMOVE_ITEM VR_SHOT_SOURCES "${CMAKE_SOURCE_DIR}/Source/Core/Window.cpp")
        add_executable(OffscreenShot "${CMAKE_SOURCE_DIR}/Tools/OffscreenShot.cpp"
                                     ${VR_SHOT_SOURCES})
        target_include_directories(OffscreenShot PRIVATE "${CMAKE_SOURCE_DIR}/Source")
        if(TARGET glm::glm)
            target_link_libraries(OffscreenShot PRIVATE glm::glm)
        elseif(DEFINED glm_SOURCE_DIR)
            target_include_directories(OffscreenShot PRIVATE "${glm_SOURCE_DIR}")
        endif()
        target_link_libraries(OffscreenShot PRIVATE ${VR_EGL_LIB} Threads::Threads
                                                    ${CMAKE_DL_LIBS} m)
    else()
        message(STATUS "OffscreenShot skipped: libEGL not found")
    endif()
endif()

# -----------------------------------------------------------------------------
#  Optional: Game smoke test (Tools/GameSmokeTest.cpp)
#  Substitutes its own GLFW entry points backed by EGL and drives the real Game
#  loop through every state. Catches crashes a gameplay-only test cannot reach.
# -----------------------------------------------------------------------------
if(VR_BUILD_SMOKE_TEST AND UNIX AND NOT APPLE)
    find_library(VR_EGL_LIB EGL)
    if(VR_EGL_LIB)
        set(VR_SMOKE_SOURCES ${VR_SOURCES})
        list(REMOVE_ITEM VR_SMOKE_SOURCES "${CMAKE_SOURCE_DIR}/Source/Main.cpp")
        add_executable(GameSmokeTest "${CMAKE_SOURCE_DIR}/Tools/GameSmokeTest.cpp"
                                     ${VR_SMOKE_SOURCES})
        target_include_directories(GameSmokeTest PRIVATE "${CMAKE_SOURCE_DIR}/Source")
        # glfw only for its headers - the test supplies the implementations.
        target_include_directories(GameSmokeTest PRIVATE
            $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>)
        if(TARGET glm::glm)
            target_link_libraries(GameSmokeTest PRIVATE glm::glm)
        elseif(DEFINED glm_SOURCE_DIR)
            target_include_directories(GameSmokeTest PRIVATE "${glm_SOURCE_DIR}")
        endif()
        target_link_libraries(GameSmokeTest PRIVATE ${VR_EGL_LIB} Threads::Threads
                                                    ${CMAKE_DL_LIBS} m)
    endif()
endif()

# Keep the shaders next to the binary as well, so the game also runs when the
# build folder is copied somewhere else.
add_custom_command(TARGET VacuumRacing3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Shaders"
            "$<TARGET_FILE_DIR:VacuumRacing3D>/Shaders")

message(STATUS "----------------------------------------------------")
message(STATUS " Vacuum Racing 3D configured")
message(STATUS "   build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "   sources    : ${CMAKE_SOURCE_DIR}/Source")
message(STATUS "----------------------------------------------------")
