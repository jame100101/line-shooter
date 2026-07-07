#include "game/Combat.hpp"
#include "game/Player.hpp"
#include "game/Enemy.hpp"
#include "game/World.hpp"
#include "core/Config.hpp"
#include <cmath>
#include <algorithm>

namespace {
float shotHeightAt(const Player& player, float dist) {
    // The prototype is 2.5D, so use a simple eye-height ray for vertical
    // occlusion. Low cover only blocks bullets below its top; full/tall walls
    // still block regardless.
    return 0.62f + std::tan(player.pitch) * dist;
}

bool blocksShotAt(const World& world, float x, float y, float shotHeight) {
    int cx = static_cast<int>(std::floor(x));
    int cy = static_cast<int>(std::floor(y));
    int id = world.at(cx, cy);
    if (id == 0) return false;
    float h = world.wallHeight(id);
    if (h >= 0.90f) return true;          // normal/tall obstacle
    return shotHeight <= h + 0.06f;       // low cover: upper empty space passes bullets
}

// Is the aimed segment clear of blocking wall volume? Steps along the ray
// sampling the grid (0.08u step is finer than any wall so we never tunnel).
bool lineOfSight(const Player& player, const Vec2& b, const World& world) {
    Vec2 a = player.pos;
    Vec2 d = b - a;
    float len = d.length();
    if (len < 1e-4f) return true;
    Vec2 step = d * (0.08f / len);
    int n = static_cast<int>(len / 0.08f);
    Vec2 p = a;
    for (int i = 0; i < n; ++i) {
        p += step;
        float dist = (p - a).length();
        if (blocksShotAt(world, p.x, p.y, shotHeightAt(player, dist))) return false;
    }
    return true;
}
} // namespace

namespace Combat {

ShotTrace traceShot(const Player& player, const std::vector<Enemy>& enemies,
                    const World& world) {
    const Config& c = cfg();
    ShotTrace tr;
    tr.start = player.pos;
    Vec2 dir{std::cos(player.angle), std::sin(player.angle)};
    tr.end = player.pos + dir * c.weaponRange;

    float wallDist = c.weaponRange;
    const float stepLen = 0.035f;
    for (float d = stepLen; d <= c.weaponRange; d += stepLen) {
        Vec2 p = player.pos + dir * d;
        if (blocksShotAt(world, p.x, p.y, shotHeightAt(player, d))) {
            wallDist = d;
            tr.end = p;
            tr.hitWall = true;
            break;
        }
    }

    int best = -1;
    float bestDist = wallDist;
    for (size_t i = 0; i < enemies.size(); ++i) {
        const Enemy& e = enemies[i];
        if (!e.alive) continue;
        Vec2 to = e.pos - player.pos;
        float dist = to.length();
        if (dist > c.weaponRange || dist < 1e-3f || dist >= bestDist) continue;

        float ang = std::atan2(to.y, to.x);
        float diff = std::fabs(wrapAngle(ang - player.angle));
        float half = std::atan2(e.radius, dist) + c.aimAssist;
        if (diff > half) continue;
        if (!lineOfSight(player, e.pos, world)) continue;

        best = static_cast<int>(i);
        bestDist = dist;
    }

    if (best >= 0) {
        tr.hitEnemy = true;
        tr.hitWall = false;
        tr.enemyIndex = best;
        tr.end = enemies[(size_t)best].pos;
    }
    return tr;
}

ShotResult resolveShot(const Player& player, std::vector<Enemy>& enemies,
                       const World& world,
                       float horizonPx, float frameH, float verticalScale) {
    const Config& c = cfg();
    ShotResult res;
    res.fired = true;

    // The aim ray direction is the player's yaw. Recoil/pitch affect vertical
    // feel but MVP enemies are ground-plane billboards, so yaw is the hit axis.
    float aim = player.angle;

    int   best = -1;
    float bestDist = c.weaponRange;

    for (size_t i = 0; i < enemies.size(); ++i) {
        Enemy& e = enemies[i];
        if (!e.alive) continue;

        Vec2 to = e.pos - player.pos;
        float dist = to.length();
        if (dist > c.weaponRange || dist < 1e-3f) continue;

        // Angular half-size of the enemy at this distance + aim assist. You must
        // be pointing at the body to connect (intuitive, matches what you see).
        float ang = std::atan2(to.y, to.x);
        float diff = std::fabs(wrapAngle(ang - aim));
        float half = std::atan2(e.radius, dist) + c.aimAssist;
        if (diff > half) continue;

        if (!lineOfSight(player, e.pos, world)) continue; // wall in the way

        if (dist < bestDist) { bestDist = dist; best = static_cast<int>(i); }
    }

    if (best < 0) return res; // missed everything
    Enemy& e = enemies[best];

    // Headshot: test whether the center crosshair falls inside the top head
    // sphere region of the same billboard used by the renderer.
    bool headshot = false;
    if (frameH > 1.f) {
        Vec2 to = e.pos - player.pos;
        float dist = to.length();
        float spriteH = frameH * verticalScale * c.enemyRenderScale / std::max(dist, 0.05f);
        if (spriteH > 1.f) {
            float crosshairV = 0.5f - horizonPx / spriteH; // 0=top, 1=bottom
            float diff = std::fabs(wrapAngle(std::atan2(to.y, to.x) - player.angle));
            float headHalf = std::atan2(e.radius * 0.6f, dist) + c.aimAssist * 0.5f;
            if (crosshairV >= 0.02f && crosshairV <= 0.33f && diff <= headHalf)
                headshot = true;
        }
    }

    e.hitFlash = c.enemyHitFlash;
    res.hit = true;
    res.enemyIndex = best;
    res.headshot = headshot;

    if (headshot) e.hp = 0.f;
    else e.hp -= static_cast<float>(c.weaponDamage);

    if (e.hp <= 0.f) {
        e.alive = false;
        e.state = Enemy::State::Dead;
        res.kill = true;
    }
    return res;
}

} // namespace Combat
