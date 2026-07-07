#include "core/Game.hpp"
#include <opencv2/core/utils/logger.hpp>
#include <cstdio>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <timeapi.h>   // timeBeginPeriod / timeEndPeriod
#endif

// Entry point. OpenCV is only the window/framebuffer/input host; everything the
// player sees is produced by our software renderer.
int main() {
#ifdef _WIN32
    // Raise the system timer resolution to 1ms so the frame-cap sleep is precise
    // (default ~15.6ms granularity otherwise pins us well below 60 FPS).
    timeBeginPeriod(1);
#endif

    // Silence OpenCV's info logging so the console stays clean.
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    std::printf(
        "FPS MVP\n"
        "  Move: WASD   Sprint: Shift\n"
        "  Look: mouse  Turn(keys): Arrows/Q-E   Fire: LMB or J   Reload: R   Quit: Esc\n");

    Game game;
    game.run();

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 0;
}
