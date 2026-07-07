#pragma once

// Runtime key bindings. Settings UI can edit these without recompiling.
struct InputBindings {
    int forward = 'W', back = 'S', left = 'A', right = 'D';
    int sprint = 0x10;   // VK_SHIFT
    int jump = 0x20;     // VK_SPACE
    int crouch = 0x11;   // VK_CONTROL
    int reload = 'R';
    int fireKey = 'J';
};

// Per-frame input snapshot. Movement/action keys are polled from Win32
// GetAsyncKeyState (true multi-key support, unlike cv::waitKey). Mouse-look uses
// GetCursorPos + SetCursorPos to emulate pointer-lock (OpenCV cannot lock the
// cursor). Everything Win32-specific is hidden inside Input.cpp.
struct InputState {
    bool forward = false, back = false, left = false, right = false; // WASD
    bool sprint  = false;   // Shift
    bool jump    = false;   // Space
    bool crouch  = false;   // Ctrl (slide)
    bool fire    = false;   // held: left mouse / J
    bool firePressed = false; // edge: fired this frame
    bool reload  = false;   // R
    bool quit    = false;   // Esc

    bool turnLeft = false, turnRight = false; // arrow keys (keyboard turn fallback)

    float mouseDX = 0.f, mouseDY = 0.f; // mouse-look deltas (pixels)
    int mouseX = 0, mouseY = 0;         // framebuffer-local cursor position
    bool mouseInside = false;
    bool mouseLeft = false;
    bool mouseLeftPressed = false;
    int keyPressed = 0;                 // edge: virtual-key pressed this frame
};

class Input {
public:
    Input();

    // Poll current key/mouse state. When mouseLock is true, the cursor is
    // recentered to (centerX, centerY) each frame and the delta is captured.
    InputState poll(bool mouseLock, bool haveImageRect,
                    int imageX, int imageY, int imageW, int imageH,
                    int frameW, int frameH,
                    const InputBindings& bindings);

private:
    bool prevFire_ = false; // for fire edge detection
    bool prevMouseLeft_ = false;
    bool prevKeys_[256]{};
    bool warm_     = false; // skip first mouse delta after (re)centering
    bool cursorHidden_ = false;
};
