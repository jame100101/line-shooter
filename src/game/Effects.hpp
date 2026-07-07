#pragma once
#include <vector>
#include <random>
#include "core/Math.hpp"

// Screen-space particle for kill gibs (simple, always visible burst).
struct Particle {
    float x, y;       // position (px)
    float vx, vy;     // velocity (px/s)
    float life, maxLife;
    unsigned char b, g, r;
};

struct BulletTracer {
    Vec2 start, end;
    float startZ = -0.10f;
    float endZ = 0.f;
    float life, maxLife;
};

struct ImpactMark {
    Vec2 pos;
    float z = 0.f;
    float life, maxLife;
};

// Juice layer: screen shake (trauma), hit-stop (time freeze), gib particles.
class Effects {
public:
    Effects();

    void addTrauma(float t);          // additive, clamped to 1
    void addHitstop(float seconds);   // extend current freeze
    void spawnBurst(float x, float y, int count,
                    unsigned char b, unsigned char g, unsigned char r);
    void spawnTracer(const Vec2& start, const Vec2& end,
                     float startZ = -0.10f, float endZ = 0.f);
    void spawnImpactMark(const Vec2& pos, float z = 0.f);

    void update(float realDt);        // decays trauma, integrates particles

    float trauma()  const { return trauma_; }
    float hitstop() const { return hitstop_; } // >0 => gameplay is frozen

    // Camera shake output derived from current trauma (quadratic feel).
    float shakeYaw();
    float shakePitchPx();

    const std::vector<Particle>& particles() const { return particles_; }
    const std::vector<BulletTracer>& tracers() const { return tracers_; }
    const std::vector<ImpactMark>& impactMarks() const { return impactMarks_; }

private:
    float trauma_  = 0.f;
    float hitstop_ = 0.f;
    std::vector<Particle> particles_;
    std::vector<BulletTracer> tracers_;
    std::vector<ImpactMark> impactMarks_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> uni_{-1.f, 1.f};
};
