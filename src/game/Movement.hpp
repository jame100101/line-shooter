#pragma once
struct Player;
struct InputState;
class  World;

// Movement system: acceleration-based high-mobility controller (sprint, jump,
// air-control/bhop, slide). Operates on Player using the current input + world.
namespace Movement {
    void update(Player& p, const InputState& in, const World& world, float dt);
}
