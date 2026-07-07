#include "render/Renderer.hpp"
#include "render/Framebuffer.hpp"
#include "game/World.hpp"
#include "game/Player.hpp"
#include "game/Weapon.hpp"
#include "game/Enemy.hpp"
#include "game/Effects.hpp"
#include "assets/AssetManager.hpp"
#include "core/Config.hpp"
#include "core/Math.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <algorithm>

Renderer::Renderer(int w, int h) : w_(w), h_(h), raycaster_(w) {}

void Renderer::renderWorldAndActors(Framebuffer& fb, const World& world,
                                    const Player& player,
                                    const std::vector<Enemy>& enemies,
                                    const Weapon& weapon, Effects& effects,
                                    AssetManager& assets) {
    const Config& c = cfg();

    // Compose camera from two separate vertical effects:
    // - horizonPx is real look pitch/recoil/shake.
    // - viewOffsetPx is camera-height translation for jump/crouch/slide, so the
    //   world moves around the fixed crosshair instead of pitching up/down.
    float fovDeg = player.viewFovDeg > 1.f ? player.viewFovDeg : (float)c.fovDeg;
    float pitchPx = (player.pitch * c.pitchScreenScale + weapon.recoil) * (h_ / deg2rad(fovDeg));
    float shakePx = effects.shakePitchPx();
    float aspect = (float)w_ / (float)h_;
    float baseVFov = 2.f * std::atan(std::tan(deg2rad((float)c.fovDeg) * 0.5f) / aspect);
    float curVFov  = 2.f * std::atan(std::tan(deg2rad(fovDeg) * 0.5f) / aspect);
    float verticalScale = std::tan(baseVFov * 0.5f) / std::tan(curVFov * 0.5f);
    verticalScale = clampf(verticalScale, 0.45f, 1.15f);
    float viewShiftPx = player.viewShiftPx; // smooth screen-space eye-height shift
    Camera cam{ player.pos.x, player.pos.y,
               player.angle + effects.shakeYaw(),
               pitchPx + shakePx,
               0.f,
               fovDeg,
               verticalScale };

    raycaster_.render(fb, world, cam, assets);
    if (c.lineArtMode) applyLineArt(fb);
    drawEnemies(fb, world, player, enemies, cam);
    drawParticles(fb, effects);
    translateWorldView(fb, viewShiftPx);
    drawTracers(fb, effects, cam);
    drawWeapon(fb, weapon);
}

// ---- Billboard enemies with per-column depth test against the wall zbuffer ----
void Renderer::drawEnemies(Framebuffer& fb, const World& /*world*/,
                           const Player& player, const std::vector<Enemy>& enemies,
                           const Camera& cam) {
    const Config& c = cfg();
    const int W = fb.width(), H = fb.height();
    const auto& zbuf = raycaster_.depth();
    const auto& lowDepth = raycaster_.lowDepth();
    const auto& lowTop = raycaster_.lowTop();

    float dirX = std::cos(cam.angle), dirY = std::sin(cam.angle);
    float planeLen = std::tan(deg2rad(cam.fovDeg) * 0.5f);
    float planeX = -dirY * planeLen, planeY = dirX * planeLen;
    float invDet = 1.f / (planeX * dirY - dirX * planeY);
    int horizon = (int)(H * 0.5f + cam.horizonPx);

    // Sort back-to-front by distance so nearer enemies overdraw farther ones.
    std::vector<int> order;
    for (size_t i = 0; i < enemies.size(); ++i)
        if (enemies[i].alive) order.push_back((int)i);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        float da = (enemies[a].pos - player.pos).length2();
        float db = (enemies[b].pos - player.pos).length2();
        return da > db;
    });

    for (int idx : order) {
        const Enemy& e = enemies[idx];
        float spX = e.pos.x - cam.x, spY = e.pos.y - cam.y;
        float tX = invDet * (dirY * spX - dirX * spY);
        float tY = invDet * (-planeY * spX + planeX * spY); // depth (perp)
        if (tY <= 0.05f) continue; // behind camera

        int screenX = (int)((W / 2) * (1 + tX / tY));
        int spriteH = std::abs((int)(H * cam.verticalScale / tY * c.enemyRenderScale));
        int spriteW = (int)(spriteH * 0.62f);             // geometric cone aspect
        int drawStartY = -spriteH / 2 + horizon;
        int drawStartX = -spriteW / 2 + screenX;

        float flash = clampf(e.hitFlash / c.enemyHitFlash, 0.f, 1.f); // 1=just hit
        float fog = clampf(tY / c.fogFar, 0.f, 1.f);                  // depth haze
        float dim = clampf(1.2f / (1.f + tY * 0.14f), 0.35f, 1.f);   // distance dim

        for (int sx = 0; sx < spriteW; ++sx) {
            int col = drawStartX + sx;
            if (col < 0 || col >= W) continue;
            if (tY >= zbuf[col]) continue; // occluded by a wall column
            float u = (float)sx / spriteW;  // 0..1 across body
            for (int sy = 0; sy < spriteH; ++sy) {
                int row = drawStartY + sy;
                if (row < 0 || row >= H) continue;
                // Low cover only hides the part of an enemy below the cover top.
                // This fixes the previous leak where enemies behind low walls
                // were drawn full-body even though their legs should be hidden.
                if (col < (int)lowDepth.size() && tY >= lowDepth[(size_t)col] &&
                    row >= lowTop[(size_t)col]) continue;
                float v = (float)sy / spriteH; // 0..1 top->bottom

                // --- Monochrome geometric enemy: spherical head + inverted cone body ---
                float b = 0, g = 0, r = 0;
                bool draw = false;
                bool inkLine = false;

                float du = u - 0.5f;
                float headEq = (du * du) / (0.155f * 0.155f) +
                               ((v - 0.17f) * (v - 0.17f)) / (0.145f * 0.145f);
                bool inHead = headEq <= 1.f;
                bool headEdge = inHead && headEq > 0.78f;
                bool headMeridian = inHead && std::fabs(du) < 0.012f;
                bool headLatitude = inHead && std::fabs(v - 0.17f) < 0.010f;
                bool headLatitude2 = inHead &&
                    (std::fabs(v - 0.118f) < 0.008f || std::fabs(v - 0.222f) < 0.008f);
                bool headDiagonal = inHead &&
                    (std::fabs((du * 1.15f + (v - 0.17f)) - 0.010f) < 0.010f ||
                     std::fabs((du * 1.15f - (v - 0.17f)) + 0.012f) < 0.010f);
                bool headHighlight = inHead &&
                    ((du + 0.055f) * (du + 0.055f)) / (0.050f * 0.050f) +
                    ((v - 0.120f) * (v - 0.120f)) / (0.035f * 0.035f) < 1.f;
                bool eyes = false; // no facial/inner line detail: pure grey-black enemy

                // Body is an inverted triangular cone: broad shoulder ring tapering
                // to a bottom point. Curved inner contour lines sell the 3D volume.
                float top = 0.335f, bottom = 0.960f;
                float coneT = clampf((v - top) / (bottom - top), 0.f, 1.f);
                float halfW = lerpf(0.345f, 0.045f, coneT);
                bool inBody = (v >= top && v <= bottom && std::fabs(du) <= halfW);
                float bodyLocal = inBody ? std::fabs(du) / std::max(halfW, 0.001f) : 2.f;
                bool bodyEdge = inBody && bodyLocal > 0.88f;
                bool centerRidge = inBody && std::fabs(du) < 0.014f;
                bool shoulderRing = inBody && std::fabs(v - top) < 0.018f;
                bool bodyRings = inBody &&
                    (std::fabs(coneT - 0.22f) < 0.015f ||
                     std::fabs(coneT - 0.43f) < 0.014f ||
                     std::fabs(coneT - 0.64f) < 0.013f ||
                     std::fabs(coneT - 0.82f) < 0.012f);
                bool bodyContour = inBody &&
                    (std::fabs(bodyLocal - 0.24f) < 0.014f ||
                     std::fabs(bodyLocal - 0.47f) < 0.014f ||
                     std::fabs(bodyLocal - 0.70f) < 0.013f ||
                     std::fabs(std::fmod((coneT * 11.5f + du * 3.8f + 8.f), 1.f) - 0.5f) < 0.030f);
                bool bottomPoint = inBody && coneT > 0.91f;

                if (inHead) {
                    draw = true;
                    b = g = r = 86.f;
                } else if (inBody) {
                    draw = true;
                    b = g = r = 68.f;
                }

                inkLine = false;
                if (!draw) continue;

                b *= dim; g *= dim; r *= dim;           // distance dimming
                if (flash > 0.f) {                      // white hit-flash blend
                    b = lerpf(b, 255, flash);
                    g = lerpf(g, 255, flash);
                    r = lerpf(r, 255, flash);
                }
                b += (c.fogB - b) * fog;                // fog to match the world
                g += (c.fogG - g) * fog;
                r += (c.fogR - r) * fog;
                fb.px(col, row, (uchar)b, (uchar)g, (uchar)r);
            }
        }
    }
}

// ---- First-person weapon intentionally disabled: keep only world-space tracers ----
void Renderer::drawWeapon(Framebuffer& fb, const Weapon& weapon) {
    (void)fb; (void)weapon;
}

// ---- Kill gib particles (screen-space) ----
void Renderer::drawParticles(Framebuffer& fb, const Effects& effects) {
    cv::Mat& img = fb.mat();
    for (const auto& p : effects.particles()) {
        float a = clampf(p.life / p.maxLife, 0.f, 1.f); // fade with life
        int size = 1 + (int)(a * 3.f);
        cv::rectangle(img,
                      cv::Rect((int)p.x, (int)p.y, size, size),
                      cv::Scalar(p.b * a, p.g * a, p.r * a), cv::FILLED);
    }
}

namespace {
bool projectWorldPoint(const Vec2& p, float z, const Camera& cam, int W, int H,
                       float& sx, float& sy, float& depth) {
    float dirX = std::cos(cam.angle), dirY = std::sin(cam.angle);
    float planeLen = std::tan(deg2rad(cam.fovDeg) * 0.5f);
    float planeX = -dirY * planeLen, planeY = dirX * planeLen;
    float invDet = 1.f / (planeX * dirY - dirX * planeY);
    float spX = p.x - cam.x, spY = p.y - cam.y;
    float tX = invDet * (dirY * spX - dirX * spY);
    float tY = invDet * (-planeY * spX + planeX * spY);
    if (tY <= 0.03f) return false;
    sx = (W / 2.f) * (1.f + tX / tY);
    sy = H * 0.5f + cam.horizonPx - z * (H * cam.verticalScale) / tY;
    depth = tY;
    return true;
}
} // namespace

void Renderer::drawTracers(Framebuffer& fb, const Effects& effects, const Camera& cam) {
    cv::Mat& img = fb.mat();
    const int W = fb.width(), H = fb.height();
    for (const auto& t : effects.tracers()) {
        float a = clampf(t.life / t.maxLife, 0.f, 1.f);
        cv::Scalar red(0, 0, 255 * a); // pure BGR red, no outer outline/glow
        float sx, sy, sd, ex, ey, ed;
        if (!projectWorldPoint(t.start, t.startZ, cam, W, H, sx, sy, sd)) continue;
        if (!projectWorldPoint(t.end, t.endZ, cam, W, H, ex, ey, ed)) continue;
        cv::Point muzzle((int)std::round(sx), (int)std::round(sy));
        cv::Point end((int)std::round(ex), (int)std::round(ey));
        cv::line(img, muzzle, end, red, 3, cv::LINE_AA);
        cv::circle(img, end, 3, red, cv::FILLED, cv::LINE_AA);
    }
}

void Renderer::drawImpactMarks(Framebuffer& fb, const Effects& effects, const Camera& cam) {
    (void)fb; (void)effects; (void)cam;
    // Impact cross marks are intentionally disabled per current visual design.
}

void Renderer::translateWorldView(Framebuffer& fb, float dyPx) {
    int dy = (int)std::round(dyPx);
    if (dy == 0) return;
    cv::Mat& img = fb.mat();
    const int W = fb.width(), H = fb.height();
    dy = std::clamp(dy, -H + 1, H - 1);
    const cv::Scalar paper(240, 240, 240);

    if (dy > 0) {
        cv::Mat src = img(cv::Rect(0, 0, W, H - dy)).clone();
        img.setTo(paper);
        src.copyTo(img(cv::Rect(0, dy, W, H - dy)));
    } else {
        int up = -dy;
        cv::Mat src = img(cv::Rect(0, up, W, H - up)).clone();
        img.setTo(paper);
        src.copyTo(img(cv::Rect(0, 0, W, H - up)));
    }
}

// Fast monochrome posterization. Edges are now baked into procedural textures and
// polygons, so we avoid per-frame Canny at 1920x1080.
void Renderer::applyLineArt(Framebuffer& fb) {
    cv::Mat& img = fb.mat();
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < img.rows; ++y) {
        cv::Vec3b* dst = img.ptr<cv::Vec3b>(y);
        for (int x = 0; x < img.cols; ++x) {
            // Scene textures are monochrome already; using one channel avoids a
            // full-frame cvtColor allocation/copy on every frame.
            uchar v = dst[x][0];
            uchar q = (v < 64) ? 14 : (v < 126) ? 38 : (v < 190) ? 222 : 250;
            dst[x] = cv::Vec3b(q, q, q);
        }
    }
}
