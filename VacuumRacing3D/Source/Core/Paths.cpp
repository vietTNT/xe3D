#include "Paths.h"

#include <cstdio>
#include <fstream>
#include <vector>

namespace vr {
namespace paths {
namespace {

std::string g_exeDir;
std::string g_root;
bool        g_searched = false;

bool fileExists(const std::string& p) {
    std::ifstream f(p.c_str());
    return f.good();
}

std::string parentOf(const std::string& p) {
    if (p.empty()) return "";
    size_t end = p.size();
    while (end > 0 && (p[end - 1] == '/' || p[end - 1] == '\\')) --end;
    const size_t slash = p.find_last_of("/\\", end == 0 ? 0 : end - 1);
    if (slash == std::string::npos) return "";
    return p.substr(0, slash);
}

void search() {
    if (g_searched) return;
    g_searched = true;

    std::vector<std::string> bases;
    if (!g_exeDir.empty()) bases.push_back(g_exeDir);
    bases.push_back(".");

    for (const std::string& base : bases) {
        std::string cur = base;
        for (int depth = 0; depth < 6; ++depth) {
            const std::string probe = cur.empty() ? std::string("Shaders/scene.vert")
                                                  : cur + "/Shaders/scene.vert";
            if (fileExists(probe)) {
                g_root = cur.empty() ? std::string(".") : cur;
                return;
            }
            const std::string up = parentOf(cur);
            if (up == cur) break;
            cur = up.empty() ? std::string(".") : up;
        }
    }
    std::fprintf(stderr,
                 "[Paths] Could not locate the Shaders/ folder. Run the game from the project "
                 "directory or keep Shaders/ next to the executable.\n");
    g_root = ".";
}

} // namespace

void initFromExecutable(const char* argv0) {
    if (!argv0) return;
    const std::string exe(argv0);
    const size_t      slash = exe.find_last_of("/\\");
    g_exeDir = (slash == std::string::npos) ? std::string(".") : exe.substr(0, slash);
    g_searched = false;
    g_root.clear();
}

const std::string& projectRoot() {
    search();
    return g_root;
}

std::string resolve(const std::string& relative) {
    search();
    if (g_root.empty() || g_root == ".") return relative;
    return g_root + "/" + relative;
}

std::string writablePath(const std::string& filename) { return resolve(filename); }

} // namespace paths
} // namespace vr
