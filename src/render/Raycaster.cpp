#include "render/Raycaster.hpp"
#include "render/Framebuffer.hpp"
#include "game/World.hpp"
#include "assets/AssetManager.hpp"
#include "core/Config.hpp"
#include "core/Math.hpp"
#include <cmath>
#include <algorithm>

Raycaster::Raycaster(int width) : zbuffer_(width, 1e9f) {}

namespace {
// Blend a color toward the fog color by factor f (0=none, 1=full fog).
inline void applyFog(float& b, float& g, float& r, float f,
                     float fb, float fg, float fr) {
    b = b + (fb - b) * f;
    g = g + (fg - g) * f;
    r = r + (fr - r) * f;
}
} // namespace

void Raycaster::render(Framebuffer& fb, const World& world, const Camera& cam,
                       AssetManager& assets) {
    const int W = fb.width();
    const int H = fb.height();
    const Config& c = cfg();
    cv::Mat& outImg = fb.mat();
    if ((int)zbuffer_.size() != W) zbuffer_.resize((size_t)W);
    lowDepth_.assign((size_t)W, 1e9f);
    lowTop_.assign((size_t)W, H + 1);
    std::vector<int> edgeTop((size_t)W, H + 1);
    std::vector<int> edgeBottom((size_t)W, -1);
    std::vector<int> edgeId((size_t)W, 0);
    std::vector<float> edgeDepth((size_t)W, 1e9f);

    // Camera basis: direction + image plane (plane length = tan(fov/2)).
    float dirX = std::cos(cam.angle), dirY = std::sin(cam.angle);
    float planeLen = std::tan(deg2rad(cam.fovDeg) * 0.5f);
    float planeX = -dirY * planeLen, planeY = dirX * planeLen;

    // Horizon line only follows actual look pitch/recoil/shake. Jump/crouch use
    // camera-height translation so the crosshair remains the projection anchor.
    float horizonF = H * 0.5f + cam.horizonPx;
    int   horizon  = (int)horizonF;
    float posZ     = c.camHeightFrac * H * cam.verticalScale; // vertical FOV scale

    const float fB = c.fogB, fG = c.fogG, fR = c.fogR, fogFar = c.fogFar;

    const cv::Mat& floorTex = assets.floorTexture();
    const int fTW = floorTex.cols, fTH = floorTex.rows; // power of two

    // ---- Open sky + perspective floor cast (rows are independent) ----
    // Outdoor-style scene: the upper half is white sky instead of a costly
    // ceiling texture; obstacles still provide the maze structure.
    int skyEnd = std::clamp(horizon, 0, H - 1);
    #pragma omp parallel for schedule(static)
    for (int y = 0; y <= skyEnd; ++y) {
        uchar* out = outImg.ptr<uchar>(y);
        for (int x = 0; x < W; ++x) {
            uchar* op = out + x * 3;
            op[0] = op[1] = op[2] = 252;
        }
    }

    // ---- Perspective floor cast ----
    // For each row we know its ground distance, walk the world XZ across the row
    // and sample the tile texture. This replaces flat fills and is what makes the
    // scene read as real 3D (converging, textured ground plane) instead of flat.
    float rdx0 = dirX - planeX, rdy0 = dirY - planeY; // ray at screen-left
    float rdx1 = dirX + planeX, rdy1 = dirY + planeY; // ray at screen-right
    #pragma omp parallel for schedule(static)
    for (int y = std::max(horizon + 1, 0); y < H; ++y) {
        float p = y - horizonF; // dist from horizon
        if (p < 1.f) p = 1.f;                    // guard: avoid div blow-up
        float rowDist = posZ / p;                // world distance of this row

        float stepX = rowDist * (rdx1 - rdx0) / W;
        float stepY = rowDist * (rdy1 - rdy0) / W;
        float wx = cam.x + rowDist * rdx0;
        float wy = cam.y + rowDist * rdy0;

        const cv::Mat& tex = floorTex;
        const uchar* td = tex.data; size_t ts = tex.step;
        uchar* out = outImg.ptr<uchar>(y);

        float fog = clampf(rowDist / fogFar, 0.f, 1.f);
        // Distance ambient dimming (nearer = brighter) for extra depth cueing.
        float shade = clampf(1.15f / (1.f + rowDist * 0.14f), 0.22f, 1.f);

        for (int x = 0; x < W; ++x) {
            int tx = ((int)((wx + 2048.f) * fTW)) & (fTW - 1);
            int ty = ((int)((wy + 2048.f) * fTH)) & (fTH - 1);
            wx += stepX; wy += stepY;
            const uchar* tp = td + (size_t)ty * ts + tx * 3;
            // High-contrast line-art mode posterizes later, so avoid per-pixel
            // float fog/shade work in this hot floor/ceiling path.
            uchar* op = out + x * 3;
            op[0] = tp[0]; op[1] = tp[1]; op[2] = tp[2];
        }
    }

    const cv::Mat& tex = assets.wallTexture(1);
    const int texW = tex.cols, texH = tex.rows;

    // ---- Per-column multi-hit DDA wall cast ----
    // Low walls no longer terminate the ray visually. We collect a few
    // solid/ramp hits along the ray and draw far->near, so the upper part above a
    // low obstacle can still show taller obstacles behind it instead of plain sky.
    #pragma omp parallel for schedule(static)
    for (int col = 0; col < W; ++col) {
        struct Hit { float perp, wallX; int side, id; };
        Hit hits[10];
        int hitCount = 0;

        float cameraX = 2.f * col / W - 1.f;
        float rayX = dirX + planeX * cameraX;
        float rayY = dirY + planeY * cameraX;

        int mapX = (int)std::floor(cam.x);
        int mapY = (int)std::floor(cam.y);

        float deltaX = (rayX == 0.f) ? 1e30f : std::fabs(1.f / rayX);
        float deltaY = (rayY == 0.f) ? 1e30f : std::fabs(1.f / rayY);

        int stepX, stepY;
        float sideDistX, sideDistY;
        if (rayX < 0) { stepX = -1; sideDistX = (cam.x - mapX) * deltaX; }
        else          { stepX =  1; sideDistX = (mapX + 1.f - cam.x) * deltaX; }
        if (rayY < 0) { stepY = -1; sideDistY = (cam.y - mapY) * deltaY; }
        else          { stepY =  1; sideDistY = (mapY + 1.f - cam.y) * deltaY; }

        float nearestFull = 1e9f;
        for (int guard = 0; guard < 512 && hitCount < 10; ++guard) {
            int side = 0;
            if (sideDistX < sideDistY) { sideDistX += deltaX; mapX += stepX; side = 0; }
            else                       { sideDistY += deltaY; mapY += stepY; side = 1; }
            int hitId = world.at(mapX, mapY);
            if (hitId == 0) continue;

            float perp = (side == 0) ? (sideDistX - deltaX) : (sideDistY - deltaY);
            if (perp < 1e-4f) perp = 1e-4f;
            if (perp > c.weaponRange * 1.7f) break;

            float wallX = (side == 0) ? (cam.y + perp * rayY) : (cam.x + perp * rayX);
            wallX -= std::floor(wallX);
            hits[hitCount++] = Hit{perp, wallX, side, hitId};

            float vh = world.wallHeight(hitId);
            if (vh >= 0.72f) {
                if (nearestFull > 1e8f) nearestFull = perp;
                break; // a full-height wall visually terminates this ray
            }
        }
        zbuffer_[col] = nearestFull;

        // Draw far->near. Near low walls cover only their lower screen interval,
        // leaving high walls behind visible in the upper interval.
        for (int hi = hitCount - 1; hi >= 0; --hi) {
            const Hit& h = hits[hi];
            const cv::Mat& tex = assets.wallTexture(h.id);
            const int texW = tex.cols, texH = tex.rows;
            int texX = (int)(h.wallX * texW);
            if ((h.side == 0 && rayX > 0) || (h.side == 1 && rayY < 0)) texX = texW - texX - 1;
            texX = std::clamp(texX, 0, texW - 1);
            // Do not thicken texture-edge columns. Obstacle corner/edge lines
            // are added in a dedicated screen-space pass below, exactly 1 px
            // wide like the floor grid lines.
            bool verticalEdge = false;

            int baseLineH = (int)(H * cam.verticalScale / h.perp);
            float visualH = world.wallHeight(h.id);
            int lineH = std::max(1, (int)(baseLineH * visualH));
            int drawEnd = (int)(baseLineH / 2 + horizon);
            int drawStart = drawEnd - lineH;
            int y0 = std::max(drawStart, 0);
            int y1 = std::min(drawEnd, H - 1);
            if (y0 > y1) continue;

            if (hi == 0) {
                edgeTop[(size_t)col] = y0;
                edgeBottom[(size_t)col] = y1;
                edgeId[(size_t)col] = h.id;
                edgeDepth[(size_t)col] = h.perp;
            }

            float texStep = (float)texH / lineH;
            float texPos = (y0 - drawStart) * texStep;
            const uchar* texData = tex.data;
            size_t texStride = tex.step;
            for (int y = y0; y <= y1; ++y) {
                int texY = std::clamp((int)texPos, 0, texH - 1);
                texPos += texStep;
                const uchar* tp = texData + (size_t)texY * texStride + texX * 3;
                if (verticalEdge) fb.px(col, y, 8, 8, 8);
                else fb.px(col, y, tp[0], tp[1], tp[2]);
            }
            if (drawStart >= 0 && drawStart < H) fb.px(col, drawStart, 8, 8, 8);
            if (drawEnd >= 0 && drawEnd < H) fb.px(col, drawEnd, 8, 8, 8);

            // Visible top cap for outdoor obstacles. This fixes the old blank
            // white area above low cover by giving the block a real top surface
            // while still allowing taller geometry behind it to show above.
            if (visualH < 1.50f) {
                float backDist = h.perp + 0.85f;
                int backBaseH = std::max(1, (int)(H * cam.verticalScale / backDist));
                int backEnd = (int)(backBaseH / 2 + horizon);
                int backTop = backEnd - std::max(1, (int)(backBaseH * visualH));
                int capY0 = std::clamp(std::min(backTop, drawStart), 0, H - 1);
                int capY1 = std::clamp(std::max(backTop, drawStart), 0, H - 1);
                if (capY0 <= capY1) {
                    for (int y = capY0; y <= capY1; ++y) {
                        bool edge = (y == capY0 || y == capY1 || verticalEdge);
                        bool diag = (((col + y) % 27) == 0);
                        uchar v = edge ? 18 : (diag ? 30 : 248);
                        fb.px(col, y, v, v, v);
                    }
                    if (capY0 >= 0 && capY0 < H) fb.px(col, capY0, 8, 8, 8);
                    if (capY1 >= 0 && capY1 < H) fb.px(col, capY1, 8, 8, 8);
                }
                if (visualH < 0.72f && h.perp < lowDepth_[(size_t)col]) {
                    lowDepth_[(size_t)col] = h.perp;
                    lowTop_[(size_t)col] = capY0;
                }
            }
        }
    }

    // Thin screen-space silhouette/corner pass. The previous texture-column
    // approach made vertical building ribs several pixels wide at close range.
    // Here every detected wall boundary is a single framebuffer column, matching
    // the procedural floor line thickness.
    for (int col = 1; col < W; ++col) {
        bool a = edgeBottom[(size_t)(col - 1)] >= 0;
        bool b = edgeBottom[(size_t)col] >= 0;
        if (!a && !b) continue;

        bool boundary = (a != b);
        if (a && b) {
            float d0 = edgeDepth[(size_t)(col - 1)];
            float d1 = edgeDepth[(size_t)col];
            float nearD = std::min(d0, d1);
            boundary = (edgeId[(size_t)(col - 1)] != edgeId[(size_t)col]) ||
                       (std::fabs(d0 - d1) > std::max(0.18f, nearD * 0.09f));
        }
        if (!boundary) continue;

        // Use only one visible segment for the boundary instead of taking the
        // union of both neighbouring columns. The union could make a low-cover
        // edge inherit the height of a taller wall behind/beside it, producing a
        // black rib that extends above the obstacle.
        int srcCol = col;
        if (a && !b) {
            srcCol = col - 1;
        } else if (!a && b) {
            srcCol = col;
        } else {
            srcCol = (edgeDepth[(size_t)(col - 1)] <= edgeDepth[(size_t)col])
                         ? col - 1
                         : col;
        }

        int top = edgeTop[(size_t)srcCol];
        int bottom = edgeBottom[(size_t)srcCol];
        top = std::clamp(top, 0, H - 1);
        bottom = std::clamp(bottom, 0, H - 1);
        for (int y = top; y <= bottom; ++y) fb.px(col, y, 18, 18, 18);
    }

}
