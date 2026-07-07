#pragma once
#include <opencv2/core.hpp>

// Framebuffer wraps the single cv::Mat we render into. OpenCV is used here ONLY
// as a pixel buffer + blit target, never as a renderer.
class Framebuffer {
public:
    Framebuffer(int w, int h)
        : w_(w), h_(h), img_(h, w, CV_8UC3, cv::Scalar(0, 0, 0)) {}

    int width()  const { return w_; }
    int height() const { return h_; }

    cv::Mat&       mat()       { return img_; }
    const cv::Mat& mat() const { return img_; }

    void resize(int w, int h) {
        w_ = w; h_ = h;
        img_.create(h_, w_, CV_8UC3);
        img_.setTo(cv::Scalar(0, 0, 0));
    }

    void clear(int b, int g, int r) { img_.setTo(cv::Scalar(b, g, r)); }

    // Fast unchecked pixel write (caller guarantees bounds). BGR order.
    inline void px(int x, int y, uchar b, uchar g, uchar r) {
        uchar* p = img_.data + (static_cast<size_t>(y) * img_.step + x * 3);
        p[0] = b; p[1] = g; p[2] = r;
    }

private:
    int w_, h_;
    cv::Mat img_;
};
