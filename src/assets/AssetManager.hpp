#pragma once
#include <unordered_map>
#include <string>
#include <opencv2/core.hpp>

// AssetManager: central texture/sprite cache. MVP generates procedural textures
// (no external files, no license risk), but the load path is stubbed so real
// CC0 art can drop in later without touching call sites.
class AssetManager {
public:
    AssetManager();

    // Wall texture for a given wall id (procedural brick for MVP).
    const cv::Mat& wallTexture(int wallId);

    // Floor / ceiling textures for perspective floor-casting (procedural).
    const cv::Mat& floorTexture();
    const cv::Mat& ceilingTexture();

    // FUTURE: load a real image from assets/ into the cache under `key`.
    // Returns false in MVP (kept so callers/tests can be written now).
    bool loadFromFile(const std::string& key, const std::string& path);

private:
    cv::Mat makeBrickTexture(cv::Scalar base, cv::Scalar mortar) const;
    cv::Mat makeFloorTexture() const;
    cv::Mat makeCeilTexture() const;

    std::unordered_map<int, cv::Mat> wallTex_;
    std::unordered_map<std::string, cv::Mat> named_;
    cv::Mat floorTex_, ceilTex_;
};
