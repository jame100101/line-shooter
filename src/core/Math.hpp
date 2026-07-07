#pragma once
#include <cmath>

// Minimal 2D vector + math helpers (header-only). No OpenCV dependency here so
// game logic stays render-agnostic.
struct Vec2 {
    float x = 0.f, y = 0.f;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s)       const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s)       { x *= s; y *= s; return *this; }

    float length()  const { return std::sqrt(x * x + y * y); }
    float length2() const { return x * x + y * y; }
    Vec2  normalized() const {
        float l = length();
        return (l > 1e-6f) ? Vec2{x / l, y / l} : Vec2{0.f, 0.f};
    }
};

inline float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

inline constexpr float kPi = 3.14159265358979323846f;
inline float deg2rad(float d) { return d * (kPi / 180.f); }

// Wrap an angle to (-pi, pi].
inline float wrapAngle(float a) {
    while (a >  kPi) a -= 2.f * kPi;
    while (a <= -kPi) a += 2.f * kPi;
    return a;
}
