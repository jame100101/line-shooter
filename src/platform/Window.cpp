#include "platform/Window.hpp"
#include <opencv2/highgui.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {
void centerWindowOnScreen(const std::string& title) {
#ifdef _WIN32
    HWND hwnd = FindWindowA(nullptr, title.c_str());
    if (!hwnd) return;

    RECT wr{};
    if (!GetWindowRect(hwnd, &wr)) return;
    int ww = wr.right - wr.left;
    int wh = wr.bottom - wr.top;

    HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfo(mon, &mi)) return;

    const RECT& area = mi.rcWork;
    int sw = area.right - area.left;
    int sh = area.bottom - area.top;
    int x = area.left + (sw - ww) / 2;
    int y = area.top + (sh - wh) / 2;

    SetWindowPos(hwnd, nullptr, x, y, 0, 0,
                 SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
#else
    (void)title;
#endif
}
} // namespace

Window::Window(const std::string& title, int w, int h)
    : title_(title), w_(w), h_(h) {
    cv::namedWindow(title_, cv::WINDOW_NORMAL);
    cv::resizeWindow(title_, w_, h_);
    // Show a black frame once so the OS creates the actual window (needed before
    // we can query its screen rect for mouse-lock).
    cv::Mat black(h_, w_, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::imshow(title_, black);
    cv::waitKey(1);
    centerWindowOnScreen(title_);
}

Window::~Window() {
    cv::destroyWindow(title_);
}

void Window::show(const cv::Mat& frame) {
    cv::imshow(title_, frame);
}

int Window::pump(int waitMs) {
    if (waitMs <= 0) return cv::pollKey();
    return cv::waitKey(waitMs);
}

bool Window::isOpen() const {
    // WND_PROP_VISIBLE drops to <1 when the user closes the window.
    return cv::getWindowProperty(title_, cv::WND_PROP_VISIBLE) >= 1.0;
}

void Window::resize(int w, int h) {
    w_ = w; h_ = h;
    cv::setWindowProperty(title_, cv::WND_PROP_FULLSCREEN, cv::WINDOW_NORMAL);
    cv::resizeWindow(title_, w_, h_);
    cv::waitKey(1);
    centerWindowOnScreen(title_);
}

void Window::setFullscreen(bool fullscreen) {
    cv::setWindowProperty(title_, cv::WND_PROP_FULLSCREEN,
                          fullscreen ? cv::WINDOW_FULLSCREEN : cv::WINDOW_NORMAL);
    if (!fullscreen) {
        cv::resizeWindow(title_, w_, h_);
        cv::waitKey(1);
        centerWindowOnScreen(title_);
    }
}

bool Window::imageCenter(int& cx, int& cy) const {
    cv::Rect r = cv::getWindowImageRect(title_); // screen coords of image area
    if (r.width <= 0 || r.height <= 0) return false;
    cx = r.x + r.width / 2;
    cy = r.y + r.height / 2;
    return true;
}

bool Window::imageRect(cv::Rect& rect) const {
    rect = cv::getWindowImageRect(title_);
    return rect.width > 0 && rect.height > 0;
}
