#include "game/Movement.hpp"
#include "game/Player.hpp"
#include "game/World.hpp"
#include "platform/Input.hpp"
#include "core/Config.hpp"
#include <cmath>

namespace {

// Quake-style directional acceleration: only adds speed up to wishSpeed along
// wishDir, which is what gives crisp strafe/air control.
void accelerate(Vec2& vel, const Vec2& wishDir, float wishSpeed,
                float accel, float dt) {
    float current = dot(vel, wishDir);
    float add = wishSpeed - current;
    if (add <= 0.f) return;
    float accelSpeed = accel * dt * wishSpeed;
    if (accelSpeed > add) accelSpeed = add;
    vel += wishDir * accelSpeed;
}

void applyFriction(Vec2& vel, float friction, float dt) {
    float speed = vel.length();
    if (speed < 1e-4f) { vel = Vec2{0, 0}; return; }
    float drop = speed * friction * dt;
    float newSpeed = std::max(speed - drop, 0.f) / speed;
    vel *= newSpeed;
}

// Move with per-axis wall sliding so we glide along walls instead of sticking.
void moveWithCollision(Player& p, const Vec2& delta, const World& world) {
    const float r = cfg().playerRadius;
    float nx = p.pos.x + delta.x;
    if (!world.circleBlocked(nx, p.pos.y, r)) p.pos.x = nx; else p.vel.x = 0.f;
    float ny = p.pos.y + delta.y;
    if (!world.circleBlocked(p.pos.x, ny, r)) p.pos.y = ny; else p.vel.y = 0.f;
}

// Critically-damped scalar smoothing, matching the common Unity/Source/Quake
// style "view height eases toward target" approach. This removes the old
// jump/crouch camera module completely: physics height and rendered eye height
// are decoupled, and only a vertical screen translation is produced.
float smoothDamp(float current, float target, float& velocity,
                 float smoothTime, float dt) {
    smoothTime = std::max(0.001f, smoothTime);
    float omega = 2.f / smoothTime;
    float x = omega * dt;
    float exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = current - target;
    float temp = (velocity + omega * change) * dt;
    velocity = (velocity - omega * temp) * exp;
    return target + (change + temp) * exp;
}

} // namespace

namespace Movement {

void update(Player& p, const InputState& in, const World& world, float dt) {
    const Config& c = cfg();

    // ---- Turning: keyboard (mouse handled by caller before this) ----
    if (in.turnLeft)  p.angle -= c.keyTurnSpeed * dt;
    if (in.turnRight) p.angle += c.keyTurnSpeed * dt;
    p.angle = wrapAngle(p.angle);

    // ---- Build wish direction from WASD relative to facing ----
    Vec2 fwd{std::cos(p.angle), std::sin(p.angle)};
    Vec2 rightv{-std::sin(p.angle), std::cos(p.angle)};
    Vec2 wish{0, 0};
    if (in.forward) wish += fwd;
    if (in.back)    wish -= fwd;
    if (in.right)   wish += rightv;
    if (in.left)    wish -= rightv;
    float wishLen = wish.length();
    Vec2 wishDir = (wishLen > 1e-4f) ? (wish * (1.f / wishLen)) : Vec2{0, 0};

    // Jump/crouch/slide are temporarily disabled per request. Keep sprint only.
    p.sliding = false;
    p.crouching = false;
    p.vz = 0.f;
    p.grounded = true;

    p.sprinting = in.sprint && in.forward;
    float targetSpeed = c.walkSpeed * (p.sprinting ? c.sprintMult : 1.f);

#if 0
    // ---- Slide start: grounded + sprinting + crouch + moving fast ----
    if (in.crouch && p.grounded && p.sprinting && !p.sliding &&
        p.vel.length() >= c.slideMinSpeed) {
        p.sliding = true;
        p.slideTimer = c.slideDuration;
        p.vel *= c.slideBoost; // burst of speed at slide entry
    }
    if (p.sliding) {
        p.slideTimer -= dt;
        // End slide when timer expires, crouch released, or airborne.
        if (p.slideTimer <= 0.f || !in.crouch || !p.grounded) p.sliding = false;
    }
    p.crouching = in.crouch && p.grounded && !p.sliding;
    if (p.crouching) targetSpeed *= c.crouchSpeedMult;
#endif

    // ---- Ground vs air movement ----
    if (p.grounded) {
        if (p.sliding) {
            // Low friction, no active acceleration: preserve momentum.
            applyFriction(p.vel, c.slideFriction, dt);
        } else {
            applyFriction(p.vel, c.friction, dt);
            accelerate(p.vel, wishDir, targetSpeed, c.groundAccel, dt);
        }
    } else {
        // Air: no friction + limited accel => air control / bunny-hop.
        accelerate(p.vel, wishDir, targetSpeed, c.airAccel, dt);
    }

    // Clamp horizontal speed to prevent runaway bhop.
    float sp = p.vel.length();
    if (sp > c.maxAirSpeed) p.vel *= (c.maxAirSpeed / sp);

    // ---- Jump / gravity disabled for now ----
#if 0
    if (in.jump && p.grounded) {
        p.vz = c.jumpImpulse;   // note: holding jump on landing keeps momentum
        p.grounded = false;     // (bhop) because air phase skips ground friction
    }
    if (!p.grounded) {
        p.vz -= c.gravity * dt;
        p.z  += p.vz * dt;
        if (p.z <= 0.f) { p.z = 0.f; p.vz = 0.f; p.grounded = true; }
    }
#endif

    // ---- Integrate horizontal position with collision ----
    moveWithCollision(p, p.vel * dt, world);
    p.z = 0.f;

    // ---- Rewritten jump/crouch view module: pure vertical eye-height offset ----
    float targetShift = 0.f;
    p.viewShiftPx = smoothDamp(p.viewShiftPx, targetShift, p.viewShiftVel,
                               c.viewSmoothTime, dt);
    p.camDrop = 0.f;

    // ---- Smooth sprint/slide FOV kick ----
    float targetFov = (p.sprinting || p.sliding) ? (float)c.sprintFovDeg : (float)c.fovDeg;
    p.viewFovDeg = lerpf(p.viewFovDeg, targetFov, clampf(dt * c.fovLerpSpeed, 0.f, 1.f));
}

} // namespace Movement
