#pragma once
#include <vector>
#include "core/Math.hpp"

// Grid-based test level. 0 = empty, >0 = wall type id. 1 unit = 1 cell.
class World {
public:
    World();

    int  width()  const { return w_; }
    int  height() const { return h_; }
    int  at(int cx, int cy) const;               // wall id at cell (0 if OOB->solid)
    float wallHeight(int wallId) const;          // visual height multiplier
    bool  isRampCell(int wallId) const;          // sloped obstacle visual
    float floorHeightAt(float x, float y) const; // ramp height under actor feet
    bool isSolidCell(int cx, int cy) const;      // cell blocked?
    bool isSolidAt(float x, float y) const;      // world position blocked?

    // Circle-vs-grid: is a disc of `radius` at (x,y) overlapping any wall?
    bool circleBlocked(float x, float y, float radius) const;

    Vec2 playerSpawn() const { return spawn_; }
    const std::vector<Vec2>& enemySpawns() const { return enemySpawns_; }

private:
    int w_ = 0, h_ = 0;
    std::vector<int> cells_;
    Vec2 spawn_;
    std::vector<Vec2> enemySpawns_;
};
