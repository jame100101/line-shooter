#include "game/Weapon.hpp"
#include <algorithm>

bool Weapon::tryFire() {
    if (cooldown > 0.f || reloadTimer > 0.f) return false;
    if (ammo <= 0) { reload(); return false; }
    const Config& c = cfg();
    --ammo;
    cooldown    = c.fireInterval;
    recoil     += c.recoilKick;      // accumulate pitch kick (climbs on spray)
    muzzleFlash = c.muzzleFlashTime;
    kickAnim    = 1.f;               // full sprite kick, decays in update()
    return true;
}

void Weapon::reload() {
    if (reloadTimer <= 0.f && ammo < magSize) reloadTimer = cfg().reloadTime;
}

void Weapon::update(float dt) {
    const Config& c = cfg();
    cooldown    = std::max(0.f, cooldown - dt);
    muzzleFlash = std::max(0.f, muzzleFlash - dt);
    kickAnim    = std::max(0.f, kickAnim - dt * 6.f);       // ~0.17s to settle
    recoil      = std::max(0.f, recoil - c.recoilRecover * dt * recoil); // ease out

    if (reloadTimer > 0.f) {
        reloadTimer -= dt;
        if (reloadTimer <= 0.f) ammo = magSize;
    } else if (ammo <= 0) {
        reload();
    }
}
