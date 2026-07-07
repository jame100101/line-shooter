#include "game/Effects.hpp"
#include "core/Config.hpp"
#include <algorithm>

Effects::Effects() : rng_(1337u) {} // fixed seed: deterministic, no Date/rand-time

void Effects::addTrauma(float t)        { trauma_  = clampf(trauma_ + t, 0.f, 1.f); }
void Effects::addHitstop(float seconds) { hitstop_ = std::max(hitstop_, seconds); }

void Effects::spawnBurst(float x, float y, int count,
                         unsigned char b, unsigned char g, unsigned char r) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x; p.y = y;
        p.vx = uni_(rng_) * 260.f;                 // spread horizontally (px/s)
        p.vy = uni_(rng_) * 260.f - 120.f;         // bias upward on spawn
        p.maxLife = 0.4f + (uni_(rng_) + 1.f) * 0.25f; // 0.4..0.9 s
        p.life = p.maxLife;
        p.b = b; p.g = g; p.r = r;
        particles_.push_back(p);
    }
}

void Effects::spawnTracer(const Vec2& start, const Vec2& end,
                          float startZ, float endZ) {
    BulletTracer t;
    t.start = start;
    t.end = end;
    t.startZ = startZ;
    t.endZ = endZ;
    t.maxLife = 1.0f;
    t.life = t.maxLife;
    tracers_.push_back(t);
    if (tracers_.size() > 16) tracers_.erase(tracers_.begin());
}

void Effects::spawnImpactMark(const Vec2& pos, float z) {
    ImpactMark m;
    m.pos = pos;
    m.z = z;
    m.maxLife = 8.0f;
    m.life = m.maxLife;
    impactMarks_.push_back(m);
    if (impactMarks_.size() > 64) impactMarks_.erase(impactMarks_.begin());
}

void Effects::update(float realDt) {
    const Config& c = cfg();

    // Hit-stop counts down in REAL time even while gameplay is frozen.
    hitstop_ = std::max(0.f, hitstop_ - realDt);
    // Trauma always decays in real time so shake settles during freezes too.
    trauma_  = std::max(0.f, trauma_ - c.traumaDecay * realDt);

    const float gib_gravity = 620.f; // px/s^2, gib fall
    for (auto& p : particles_) {
        p.life -= realDt;
        p.x += p.vx * realDt;
        p.y += p.vy * realDt;
        p.vy += gib_gravity * realDt;
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
                       [](const Particle& p) { return p.life <= 0.f; }),
        particles_.end());

    for (auto& t : tracers_) t.life -= realDt;
    tracers_.erase(
        std::remove_if(tracers_.begin(), tracers_.end(),
                       [](const BulletTracer& t) { return t.life <= 0.f; }),
        tracers_.end());

    for (auto& m : impactMarks_) m.life -= realDt;
    impactMarks_.erase(
        std::remove_if(impactMarks_.begin(), impactMarks_.end(),
                       [](const ImpactMark& m) { return m.life <= 0.f; }),
        impactMarks_.end());
}

float Effects::shakeYaw() {
    float amt = trauma_ * trauma_;                 // quadratic: subtle then punchy
    return uni_(rng_) * cfg().shakeMaxYaw * amt;
}
float Effects::shakePitchPx() {
    float amt = trauma_ * trauma_;
    return uni_(rng_) * cfg().shakeMaxPitch * amt;
}
