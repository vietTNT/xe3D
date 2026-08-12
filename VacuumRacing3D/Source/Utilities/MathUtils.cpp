#include "MathUtils.h"

#include <cstdio>

namespace vr {
namespace math {

void formatLapTime(float seconds, char* out, int outSize) {
    if (!out || outSize <= 0) return;
    if (seconds < 0.0f) {
        std::snprintf(out, (size_t)outSize, "--:--.---");
        return;
    }
    const int total = (int)seconds;
    const int m     = total / 60;
    const int s     = total % 60;
    const int ms    = (int)((seconds - (float)total) * 1000.0f);
    std::snprintf(out, (size_t)outSize, "%d:%02d.%03d", m, s, ms);
}

} // namespace math
} // namespace vr
