// -----------------------------------------------------------------------------
//  Paths.h - locates the Shaders/ and Assets/ folders no matter where the game
//  is launched from (build dir, install dir, IDE working directory).
// -----------------------------------------------------------------------------
#pragma once

#include <string>

namespace vr {
namespace paths {

/// Records argv[0] so the search can start next to the executable.
void initFromExecutable(const char* argv0);

/// Returns the project root that contains Shaders/ (empty when not found).
const std::string& projectRoot();

/// Resolves a project relative path, e.g. "Shaders/scene.vert".
std::string resolve(const std::string& relative);

/// Directory where settings/records are written (project root, or cwd).
std::string writablePath(const std::string& filename);

} // namespace paths
} // namespace vr
