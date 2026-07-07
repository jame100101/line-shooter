#pragma once
#include <vector>
class Framebuffer;
class World;
class AssetManager;

// Camera pose fed to the raycaster/sprite renderer for one frame.
struct Camera {
    float x, y;        // player position (world units)
    float angle;       // yaw (rad) incl. shake
    float horizonPx;   // vertical horizon offset (px): pitch/recoil/shake only
    float viewOffsetPx;// vertical screen translation for jump/crouch camera height
    float fovDeg;      // horizontal field of view (deg), may widen while sprinting
    float verticalScale;// projection scale for vertical FOV widening
};

// 2.5D raycaster: draws ceiling/wall/floor into the framebuffer and fills the
// per-column depth buffer used for sprite occlusion.
class Raycaster {
public:
    explicit Raycaster(int width);

    void render(Framebuffer& fb, const World& world, const Camera& cam,
                AssetManager& assets);

    const std::vector<float>& depth() const { return zbuffer_; }
    const std::vector<float>& lowDepth() const { return lowDepth_; }
    const std::vector<int>& lowTop() const { return lowTop_; }

private:
    std::vector<float> zbuffer_; // perpendicular wall distance per screen column
    std::vector<float> lowDepth_; // nearest low-cover distance per screen column
    std::vector<int>   lowTop_;   // screen y where low cover starts occluding
};
