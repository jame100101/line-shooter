#pragma once
#include "core/Config.hpp"

// First-person weapon: fire-rate gating, ammo/reload, recoil kick, muzzle flash.
// Actual hit resolution is done by Combat (hitscan); this only models the gun.
struct Weapon {
    int   ammo        = cfg().magSize;
    int   magSize     = cfg().magSize;
    float cooldown    = 0.f;   // time until next shot allowed (s)
    float recoil      = 0.f;   // current pitch kick (rad), decays over time
    float muzzleFlash = 0.f;   // muzzle-flash timer (s)
    float reloadTimer = 0.f;   // >0 while reloading (s)
    float kickAnim    = 0.f;   // 0..1 visual recoil for sprite (decays)

    // Attempt to fire. Returns true if a round was actually discharged.
    bool tryFire();

    void reload();
    void update(float dt);
    bool isReloading() const { return reloadTimer > 0.f; }
};
