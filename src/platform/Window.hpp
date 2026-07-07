#pragma once
#include <string>
#include <opencv2/core.hpp>

// Thin wrapper over OpenCV highgui: create window, blit framebuffer, pump the
// OS message loop, and report screen-space center (for mouse-lock recentering).
class Window {
public:
    Window(const std::string& title, int w, int h);
    ~Window();

    void show(const cv::Mat& frame);   // imshow the framebuffer
    int  pump(int waitMs = 1);         // waitKey -> keeps window responsive
    bool isOpen() const;               // false once user closes the window
    void resize(int w, int h);
    void setFullscreen(bool fullscreen);

    // Screen-space center of the image area (for SetCursorPos mouse-lock).
    // Returns false if the window rect is not yet valid.
    bool imageCenter(int& cx, int& cy) const;
    bool imageRect(cv::Rect& rect) const;

private:
    std::string title_;
    int w_, h_;
};
