#pragma once
#include "core/Math.hpp"

class World;
struct Player;

// Enemy archetypes. Grunt = standard; Rusher = fast/fragile/aggressive.
enum class EnemyType { Grunt, Rusher };

// Billboard enemy with a tiny state-machine AI (chase / strafe / attack / die).
// Per-enemy stats are baked at spawn (from Config) so multiple types coexist.
struct Enemy {
    Vec2  pos;
    float hp        = 100.f;
    float maxHp     = 100.f;
    bool  alive     = true;
    float hitFlash  = 0.f;    // white-flash timer on being hit (s)
    float attackCd  = 0.f;    // attack cooldown (s)
    float strafeDir = 1.f;    // +1/-1 circling direction

    EnemyType type  = EnemyType::Grunt;
    // Baked stats:
    float speed        = 2.2f;
    float radius       = 0.30f;
    float attackRange  = 1.3f;
    float strafeDist   = 4.5f;
    int   contactDamage= 8;
    unsigned char colB = 40, colG = 60, colR = 190; // torso color (BGR)

    enum class State { Chase, Strafe, Attack, Dead };
    State state = State::Chase;
};

// Build an enemy of the given type at pos, filling stats/color from Config.
Enemy spawnEnemy(EnemyType type, const Vec2& pos);

// Advance one enemy. Returns damage it dealt to the player this frame (0 if none).
namespace EnemyAI {
    int update(Enemy& e, const Player& player, const World& world, float dt);
}
