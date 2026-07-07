#include "game/Enemy.hpp"
#include "game/Player.hpp"
#include "game/World.hpp"
#include "core/Config.hpp"
#include <cmath>
#include <algorithm>

Enemy spawnEnemy(EnemyType type, const Vec2& pos) {
    const Config& c = cfg();
    Enemy e;
    e.pos = pos;
    e.type = type;
    if (type == EnemyType::Rusher) {
        e.hp = e.maxHp = (float)c.rusherHp;
        e.speed = c.rusherSpeed;
        e.radius = c.rusherRadius;
        e.attackRange = c.rusherAttackRange;
        e.strafeDist = c.rusherStrafeDist;
        e.contactDamage = c.rusherDamage;
        e.colB = (unsigned char)c.rusherColB;
        e.colG = (unsigned char)c.rusherColG;
        e.colR = (unsigned char)c.rusherColR;
    } else { // Grunt
        e.hp = e.maxHp = (float)c.enemyHp;
        e.speed = c.enemySpeed;
        e.radius = c.enemyRadius;
        e.attackRange = c.enemyAttackRange;
        e.strafeDist = c.enemyStrafeDist;
        e.contactDamage = c.enemyDamage;
        e.colB = (unsigned char)c.gruntColB;
        e.colG = (unsigned char)c.gruntColG;
        e.colR = (unsigned char)c.gruntColR;
    }
    return e;
}

namespace {
// Move an enemy by delta with simple per-axis wall avoidance.
void moveAvoiding(Enemy& e, const Vec2& delta, const World& world) {
    float nx = e.pos.x + delta.x;
    if (!world.circleBlocked(nx, e.pos.y, e.radius)) e.pos.x = nx;
    float ny = e.pos.y + delta.y;
    if (!world.circleBlocked(e.pos.x, ny, e.radius)) e.pos.y = ny;
}
} // namespace

namespace EnemyAI {

int update(Enemy& e, const Player& player, const World& world, float dt) {
    if (!e.alive) { e.state = Enemy::State::Dead; return 0; }

    const Config& c = cfg();
    e.hitFlash = std::max(0.f, e.hitFlash - dt);
    e.attackCd = std::max(0.f, e.attackCd - dt);

    Vec2 toP = player.pos - e.pos;
    float dist = toP.length();
    Vec2 dir = (dist > 1e-4f) ? (toP * (1.f / dist)) : Vec2{0, 0};

    int damage = 0;

    if (dist <= e.attackRange) {
        // ---- Attack: in range, strike on cooldown ----
        e.state = Enemy::State::Attack;
        if (e.attackCd <= 0.f) {
            damage = e.contactDamage;
            e.attackCd = c.enemyAttackCd;
        }
        // Small jitter so they don't perfectly overlap the player.
        moveAvoiding(e, dir * (e.speed * 0.3f * dt), world);
    } else if (dist <= e.strafeDist) {
        // ---- Strafe: circle the player while closing slightly ----
        e.state = Enemy::State::Strafe;
        Vec2 tangent{-dir.y, dir.x};
        Vec2 move = (dir * 0.5f + tangent * e.strafeDir).normalized();
        moveAvoiding(e, move * (e.speed * dt), world);
        // Occasionally flip circling direction for variety (distance-hashed).
        if (std::fmod(dist, 3.0f) < 0.02f) e.strafeDir = -e.strafeDir;
    } else {
        // ---- Chase: head straight for the player ----
        e.state = Enemy::State::Chase;
        moveAvoiding(e, dir * (e.speed * dt), world);
    }

    return damage;
}

} // namespace EnemyAI
