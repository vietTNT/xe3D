// -----------------------------------------------------------------------------
//  Dua Xe May Hut Bui / Vacuum Racing 3D
//  A complete 3D racing game written from scratch in C++ with OpenGL 3.3 core.
//
//  Dependencies: GLFW (window + input) and GLM (maths). Every mesh, texture and
//  sound in the game is generated procedurally at start-up.
// -----------------------------------------------------------------------------
#include <cstdio>

#include "Game.h"

int main(int argc, char** argv) {
    std::printf("=============================================\n");
    std::printf("  DUA XE MAY HUT BUI - VACUUM RACING 3D\n");
    std::printf("  C++ / OpenGL 3.3 Core\n");
    std::printf("=============================================\n");

    vr::Game game;
    if (!game.init(argc, argv)) {
        std::fprintf(stderr, "Startup failed. See the messages above.\n");
        return 1;
    }
    game.run();
    game.shutdown();
    std::printf("Goodbye!\n");
    return 0;
}
