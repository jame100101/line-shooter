#include "game/World.hpp"
#include <cmath>
#include <algorithm>

World::World() {
    // Expanded 32x24 arena in the same grid/line-art style:
    // border walls + blocky internal walls + pillars, keeping open sightlines.
    w_ = 32;
    h_ = 24;
    cells_.assign(static_cast<size_t>(w_) * h_, 0);

    auto setWall = [&](int x, int y, int id = 1) {
        if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
        cells_[static_cast<size_t>(y) * w_ + x] = id;
    };
    auto rect = [&](int x0, int y0, int rw, int rh, int id = 1) {
        for (int y = y0; y < y0 + rh; ++y)
            for (int x = x0; x < x0 + rw; ++x)
                setWall(x, y, id);
    };

    for (int x = 0; x < w_; ++x) { setWall(x, 0); setWall(x, h_ - 1); }
    for (int y = 0; y < h_; ++y) { setWall(0, y); setWall(w_ - 1, y); }

    // Large structural shapes with gaps so the raycaster still reads as a maze.
    rect(15, 1, 1, 7, 3);      // tall spine
    rect(15, 16, 1, 7, 3);
    rect(4, 4, 4, 2);
    rect(4, 5, 1, 5, 3);
    rect(10, 8, 5, 1, 2);      // low cover
    rect(11, 9, 1, 5, 2);
    rect(20, 3, 5, 2);
    rect(22, 5, 1, 5, 3);
    rect(24, 8, 3, 4, 2);
    rect(5, 15, 5, 2, 2);
    rect(17, 15, 4, 2);
    rect(24, 17, 4, 3, 3);
    rect(28, 9, 2, 1);
    rect(2, 11, 4, 1);
    rect(7, 20, 6, 1);
    rect(19, 21, 8, 1);

    // Small pillars/cover pieces.
    rect(8, 8, 1, 1, 2);
    rect(18, 10, 1, 1, 3);
    rect(28, 6, 1, 1, 2);
    rect(12, 20, 1, 1, 3);
    rect(3, 20, 1, 1, 2);
    rect(27, 14, 1, 1, 3);
    rect(6, 12, 1, 1, 2);


    spawn_ = Vec2(16.5f, 12.5f);
    enemySpawns_ = {
        Vec2(3.5f, 3.5f),  Vec2(12.5f, 3.5f), Vec2(28.5f, 3.5f),
        Vec2(3.5f, 18.5f), Vec2(14.5f, 20.5f), Vec2(29.5f, 19.5f),
        Vec2(8.5f, 13.5f), Vec2(21.5f, 12.5f), Vec2(29.5f, 8.5f)
    };
}

int World::at(int cx, int cy) const {
    if (cx < 0 || cy < 0 || cx >= w_ || cy >= h_) return 1; // OOB = solid
    return cells_[static_cast<size_t>(cy) * w_ + cx];
}

float World::wallHeight(int wallId) const {
    switch (wallId) {
        case 2: return 0.46f; // low cover
        case 3: return 1.36f; // tall obstacle
        default: return 1.0f;
    }
}

bool World::isRampCell(int /*wallId*/) const { return false; }

float World::floorHeightAt(float /*x*/, float /*y*/) const {
    return 0.f;
}

bool World::isSolidCell(int cx, int cy) const { return at(cx, cy) != 0; }

bool World::isSolidAt(float x, float y) const {
    return isSolidCell(static_cast<int>(std::floor(x)),
                       static_cast<int>(std::floor(y)));
}

bool World::circleBlocked(float x, float y, float radius) const {
    // Sample the four cardinal extents of the disc against the grid. Cheap and
    // sufficient for radii < 0.5 cell.
    if (isSolidAt(x + radius, y)) return true;
    if (isSolidAt(x - radius, y)) return true;
    if (isSolidAt(x, y + radius)) return true;
    if (isSolidAt(x, y - radius)) return true;
    return false;
}
