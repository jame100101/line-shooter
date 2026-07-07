#include "assets/AssetManager.hpp"
#include "core/Math.hpp"   // clampf
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <cstdlib>

AssetManager::AssetManager() {
    // Pre-bake monochrome textures: bright fills plus black seams for a
    // raylib-like black/white line-art read.
    wallTex_[1] = makeBrickTexture(cv::Scalar(246, 246, 246), cv::Scalar(20, 20, 20));
    floorTex_ = makeFloorTexture();
    ceilTex_  = makeCeilTexture();
}

const cv::Mat& AssetManager::wallTexture(int wallId) {
    auto it = wallTex_.find(wallId);
    if (it != wallTex_.end()) return it->second;
    // Fallback: flat mid-grey so an unknown id still renders something.
    if (wallTex_.find(0) == wallTex_.end())
        wallTex_[0] = cv::Mat(64, 64, CV_8UC3, cv::Scalar(246, 246, 246));
    return wallTex_[0];
}

const cv::Mat& AssetManager::floorTexture()   { return floorTex_; }
const cv::Mat& AssetManager::ceilingTexture() { return ceilTex_; }

bool AssetManager::loadFromFile(const std::string& key, const std::string& path) {
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    if (img.empty()) return false; // MVP has no files; returns false gracefully
    named_[key] = img;
    return true;
}

// 64x64 procedural brick: base fill + offset-row mortar lines + subtle noise.
cv::Mat AssetManager::makeBrickTexture(cv::Scalar base, cv::Scalar mortar) const {
    const int S = 128;
    cv::Mat t(S, S, CV_8UC3, base);
    cv::Scalar ink(mortar[0], mortar[1], mortar[2]);
    cv::Scalar diag(18, 18, 18);

    // Side-face pattern for vertical obstacles: each visible side is a tall
    // rectangle with the A/Z-like internal lines from the reference sketch.
    // Top/bottom surfaces remain controlled by floor/open-sky rendering.
    // Only draw the horizontal caps in the texture. Vertical/corner edges are
    // drawn later in screen space as single-pixel columns; baking them into the
    // texture makes close obstacle corners balloon into thick black bars.
    cv::line(t, {0, 0}, {S - 1, 0}, ink, 1, cv::LINE_8);
    cv::line(t, {0, S - 1}, {S - 1, S - 1}, ink, 1, cv::LINE_8);
    cv::line(t, {S / 2, 0}, {S / 4, S - 1}, diag, 2, cv::LINE_8);
    cv::line(t, {S / 2, 0}, {S * 3 / 4, S - 1}, diag, 2, cv::LINE_8);
    cv::line(t, {S * 3 / 4, S - 1}, {S - 1, 0}, diag, 2, cv::LINE_8);

    // Slight paper grain, kept light so the black structural lines dominate.
    for (int y = 0; y < S; ++y) {
        for (int x = 0; x < S; ++x) {
            cv::Vec3b& col = t.at<cv::Vec3b>(y, x);
            if (col[0] < 80) continue;
            int n = ((x * 13 + y * 7) % 5) - 2;
            col = cv::Vec3b((uchar)clampf((float)col[0] + n, 0, 255),
                            (uchar)clampf((float)col[1] + n, 0, 255),
                            (uchar)clampf((float)col[2] + n, 0, 255));
        }
    }
    return t;
}

// 64x64 stone-tile floor: 2x2 tiles with grout seams + deterministic speckle.
// Perspective floor-casting samples this, which is what sells the "real 3D" look.
cv::Mat AssetManager::makeFloorTexture() const {
    const int S = 128;
    cv::Mat t(S, S, CV_8UC3);
    const int tile = 64, grout = 1;               // larger tiles, same 1px seams
    cv::Vec3b base(248, 248, 248), alt(236, 236, 236), seam(18, 18, 18);
    for (int y = 0; y < S; ++y) {
        for (int x = 0; x < S; ++x) {
            int lx = x % tile, ly = y % tile;
            bool seamPx = lx < grout || ly < grout;
            cv::Vec3b col;
            bool hatch = (lx == ly); // one diagonal per tile
            if (seamPx) col = seam;
            else {
                bool checker = ((x / tile) + (y / tile)) & 1;
                cv::Vec3b b = checker ? alt : base;
                int n = ((x * 17 + y * 11) % 5) - 2; // -2..2 speckle
                col = hatch ? cv::Vec3b(30, 30, 30) : cv::Vec3b((uchar)clampf(b[0] + n, 0, 255),
                                (uchar)clampf(b[1] + n, 0, 255),
                                (uchar)clampf(b[2] + n, 0, 255));
            }
            t.at<cv::Vec3b>(y, x) = col;
        }
    }
    return t;
}

// 64x64 dark panelled ceiling: subtle grid so distant ceiling still reads depth.
cv::Mat AssetManager::makeCeilTexture() const {
    const int S = 128;
    cv::Mat t(S, S, CV_8UC3, cv::Scalar(248, 248, 248));
    const int panel = 64;
    cv::Vec3b line(18, 18, 18), hatch(34, 34, 34);
    for (int y = 0; y < S; ++y)
        for (int x = 0; x < S; ++x) {
            int lx = x % panel, ly = y % panel;
            if (lx < 1 || ly < 1)
                t.at<cv::Vec3b>(y, x) = line;
            else if (lx == ly)
                t.at<cv::Vec3b>(y, x) = hatch;
        }
    return t;
}
