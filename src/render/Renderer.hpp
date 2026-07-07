#pragma once
#include "render/Raycaster.hpp"

class  Framebuffer;
class  World;
class  AssetManager;
struct Player;
struct Weapon;
struct Enemy;
class  Effects;
#include <vector>

// Renderer orchestrates the full frame: world (raycaster) -> billboard enemies
// (depth-tested) -> first-person weapon (recoil + muzzle flash) -> gib particles.
// The HUD overlay is drawn separately by Hud after this.
class Renderer {
public:
    Renderer(int w, int h);

    void renderWorldAndActors(Framebuffer& fb, const World& world,
                              const Player& player, const std::vector<Enemy>& enemies,
                              const Weapon& weapon, Effects& effects,
                              AssetManager& assets);

private:
    void drawEnemies(Framebuffer& fb, const World& world, const Player& player,
                     const std::vector<Enemy>& enemies, const Camera& cam);
    void drawWeapon(Framebuffer& fb, const Weapon& weapon);
    void drawParticles(Framebuffer& fb, const Effects& effects);
    void drawTracers(Framebuffer& fb, const Effects& effects, const Camera& cam);
    void drawImpactMarks(Framebuffer& fb, const Effects& effects, const Camera& cam);
    void translateWorldView(Framebuffer& fb, float dyPx);
    void applyLineArt(Framebuffer& fb);

    int w_, h_;
    Raycaster raycaster_;
};
