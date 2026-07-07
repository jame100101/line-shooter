#pragma once
#include <chrono>
#include <thread>

// Small timing helpers built on std::chrono (header-only).
namespace timeutil {

using clock = std::chrono::steady_clock;

inline clock::time_point now() { return clock::now(); }

// Seconds elapsed between two time points.
inline double seconds(clock::time_point a, clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}

// Sleep for the given number of seconds (used for frame capping).
inline void sleepSeconds(double s) {
    if (s > 0.0)
        std::this_thread::sleep_for(std::chrono::duration<double>(s));
}

} // namespace timeutil
