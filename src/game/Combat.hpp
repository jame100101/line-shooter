#pragma once
#include <vector>
#include "core/Math.hpp"
struct Player;
struct Enemy;
class  World;

// Result of a single trigger pull, used to drive feedback (crosshair, shake...).
struct ShotResult {
    bool fired = false;  // did the gun actually discharge?
    bool hit   = false;  // did it hit an enemy?
    bool kill  = false;  // did that hit kill the enemy?
    bool headshot = false; // did the crosshair land on the enemy head sphere?
    int  enemyIndex = -1;// which enemy (into the enemies vector), if any
};

struct ShotTrace {
    Vec2 start;
    Vec2 end;
    bool hitEnemy = false;
    bool hitWall = false;
    int enemyIndex = -1;
};

// Hitscan combat: casts a ray along the player's aim and damages the nearest
// enemy whose body the crosshair covers (with line-of-sight + small aim assist).
namespace Combat {
    ShotTrace traceShot(const Player& player,
                        const std::vector<Enemy>& enemies,
                        const World& world);

    ShotResult resolveShot(const Player& player,
                           std::vector<Enemy>& enemies,
                           const World& world,
                           float horizonPx = 0.f, float frameH = 0.f,
                           float verticalScale = 1.f);
}
