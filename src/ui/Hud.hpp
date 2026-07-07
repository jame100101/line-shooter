#pragma once
class Framebuffer;
struct Player;
struct Weapon;
#include <string>

// Minimal boomer-shooter HUD: crosshair, HP, Ammo, kill count, kill/streak pops.
// Holds only transient UI timers; all drawing goes straight to the framebuffer.
class Hud {
public:
    // Feedback signals raised by gameplay this frame.
    void onHit();               // crosshair hit-marker flash
    void onKill();              // "KILL" pop + killstreak logic

    void update(float dt);
    void draw(Framebuffer& fb, const Player& player, const Weapon& weapon,
              int kills, float fps);

private:
    float hitMarker_   = 0.f;   // hit-marker timer (s)
    float killPopup_   = 0.f;   // "KILL" popup timer (s)
    int   streak_      = 0;     // current killstreak count
    float streakTimer_ = 0.f;   // time window to keep the streak alive (s)
    std::string streakText_;    // DOUBLE / TRIPLE KILL ...
};
