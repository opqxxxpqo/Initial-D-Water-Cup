#pragma once
#include <cmath>
#include "water_core.h"

namespace cup {
// First-stage deterministic bench stimulus. Future IMU adapter should return
// the same MotionInput contract after orientation mapping and calibration.
inline spill::MotionInput simulatedMotion(double seconds) {
    spill::MotionInput input;
    input.timestampUs = uint64_t(seconds * 1000000.0);
    // Start the existing motion cycle in its active phase, so the opening
    // frame does not spend a second at rest before the first visible slosh.
    const double cycle = std::fmod(seconds + 1.5, 12.0);
    auto smooth=[](double t){if(t<0)t=0;if(t>1)t=1;return t*t*(3.0-2.0*t);};
    const double gain=smooth((cycle-.2)/1.3)*smooth((10.5-cycle)/2.0);
    input.xG=gain*.75*std::sin((cycle-1.0)*2.6);
    input.yG=gain*.32*std::sin((cycle-1.0)*1.7+.3);
    return input;
}
} // namespace cup
