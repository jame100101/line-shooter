#include "ui/Hud.hpp"
#include "render/Framebuffer.hpp"
#include "game/Player.hpp"
#include "game/Weapon.hpp"
#include "core/Config.hpp"
#include "core/Math.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cstdio>

namespace {
const float kHitMarkerTime = 0.12f; // crosshair hit flash (s)
const float kKillPopupTime = 0.9f;  // "KILL" stays up (s)
const float kStreakWindow  = 2.0f;  // chain kills within this window (s)
const cv::Scalar kUiRed(36, 48, 224);       // BGR, pixel-HUD red
const cv::Scalar kUiRedDark(12, 18, 96);

cv::Rect clampRect(const cv::Rect& r, int W, int H) {
    return r & cv::Rect(0, 0, W, H);
}

void blurPanel(cv::Mat& img, cv::Rect r) {
    r = clampRect(r, img.cols, img.rows);
    if (r.empty()) return;
    cv::Mat src = img(r).clone();
    cv::Mat blurred;
    cv::GaussianBlur(src, blurred, cv::Size(0, 0), 3.0);
    cv::Mat paper(src.size(), src.type(), cv::Scalar(238, 238, 238));
    cv::addWeighted(blurred, 0.42, paper, 0.58, 0.0, img(r));
    cv::rectangle(img, r, cv::Scalar(250, 250, 250), 1, cv::LINE_8);
}

void pixelText(cv::Mat& img, const std::string& s, cv::Point org, double scale,
               cv::Scalar color = kUiRed, int thick = 2) {
    // LINE_8 + shadow gives the intentionally blocky, low-res red HUD look.
    cv::putText(img, s, org + cv::Point(3, 3), cv::FONT_HERSHEY_SIMPLEX,
                scale, kUiRedDark, thick + 1, cv::LINE_8);
    cv::putText(img, s, org, cv::FONT_HERSHEY_SIMPLEX,
                scale, color, thick, cv::LINE_8);
}
} // namespace

void Hud::onHit() { hitMarker_ = kHitMarkerTime; }

void Hud::onKill() {
    killPopup_ = kKillPopupTime;
    if (streakTimer_ > 0.f) ++streak_; else streak_ = 1;
    streakTimer_ = kStreakWindow;
    switch (streak_) {
        case 2:  streakText_ = "DOUBLE KILL"; break;
        case 3:  streakText_ = "TRIPLE KILL"; break;
        case 4:  streakText_ = "MULTI KILL";  break;
        default: streakText_ = (streak_ >= 5) ? "RAMPAGE" : ""; break;
    }
}

void Hud::update(float dt) {
    hitMarker_   = std::max(0.f, hitMarker_ - dt);
    killPopup_   = std::max(0.f, killPopup_ - dt);
    streakTimer_ = std::max(0.f, streakTimer_ - dt);
    if (streakTimer_ <= 0.f) streak_ = 0; // streak expired
}

void Hud::draw(Framebuffer& fb, const Player& player, const Weapon& weapon,
               int kills, float fps) {
    cv::Mat& img = fb.mat();
    const int W = fb.width(), H = fb.height();
    const int cx = W / 2, cy = H / 2;

    // ---- Crosshair: still black/white so the red tracer and red HUD remain distinct ----
    bool hit = hitMarker_ > 0.f;
    cv::Scalar chCol = hit ? kUiRed : cv::Scalar(8, 8, 8);
    int gap = 5, len = hit ? 14 : 10;
    cv::line(img, {cx - gap - len, cy}, {cx - gap, cy}, chCol, 2, cv::LINE_AA);
    cv::line(img, {cx + gap, cy}, {cx + gap + len, cy}, chCol, 2, cv::LINE_AA);
    cv::line(img, {cx, cy - gap - len}, {cx, cy - gap}, chCol, 2, cv::LINE_AA);
    cv::line(img, {cx, cy + gap}, {cx, cy + gap + len}, chCol, 2, cv::LINE_AA);
    cv::circle(img, {cx, cy}, 1, chCol, cv::FILLED);
    if (hit) {
        int s = 10;
        cv::line(img, {cx - s, cy - s}, {cx + s, cy + s}, chCol, 2, cv::LINE_AA);
        cv::line(img, {cx - s, cy + s}, {cx + s, cy - s}, chCol, 2, cv::LINE_AA);
    }

    // ---- Pixel red UI on blurred-paper strips ----
    char hpBuf[32]; std::snprintf(hpBuf, sizeof hpBuf, "HP: %d", std::max(0, player.hp));
    blurPanel(img, cv::Rect(24, H - 92, 260, 58));
    pixelText(img, hpBuf, {42, H - 52}, 1.05, kUiRed, 2);

    char amBuf[48];
    if (weapon.isReloading()) std::snprintf(amBuf, sizeof amBuf, "RELOAD");
    else std::snprintf(amBuf, sizeof amBuf, "%02d / %02d", weapon.ammo, weapon.magSize);
    blurPanel(img, cv::Rect(W - 310, H - 88, 280, 58));
    pixelText(img, amBuf, {W - 288, H - 49}, 1.00, kUiRed, 2);

    char kBuf[40]; std::snprintf(kBuf, sizeof kBuf, "KILL: %d", kills);
    blurPanel(img, cv::Rect(24, 20, 250, 54));
    pixelText(img, kBuf, {42, 58}, 0.95, kUiRed, 2);

    char fBuf[24]; std::snprintf(fBuf, sizeof fBuf, "%.0f FPS", fps);
    blurPanel(img, cv::Rect(W - 170, 20, 146, 42));
    pixelText(img, fBuf, {W - 154, 50}, 0.55, kUiRed, 1);

    // ---- Kill popup (center-top) ----
    if (killPopup_ > 0.f) {
        float a = clampf(killPopup_ / kKillPopupTime, 0.f, 1.f);
        int base = cy - 74 - (int)((1 - a) * 20);
        pixelText(img, "KILL", {cx - 48, base}, 1.0, kUiRed, 2);
        if (streak_ >= 2 && !streakText_.empty()) {
            int tw = (int)streakText_.size() * 18;
            pixelText(img, streakText_, {cx - tw / 2, base + 38}, 0.62, kUiRed, 1);
        }
    }
}
