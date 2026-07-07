#include "platform/Input.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace {
// True if the given virtual-key is currently down.
inline bool down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

const int kBindableKeys[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    '0','1','2','3','4','5','6','7','8','9',
    VK_SPACE, VK_SHIFT, VK_CONTROL, VK_MENU, VK_TAB,
    VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT,
    VK_LBUTTON, VK_RBUTTON
};

void setCursorVisible(bool visible, bool& hiddenState) {
    if (visible && hiddenState) {
        while (ShowCursor(TRUE) < 0) {}
        hiddenState = false;
    } else if (!visible && !hiddenState) {
        while (ShowCursor(FALSE) >= 0) {}
        hiddenState = true;
    }
}
} // namespace

Input::Input() {
    // Start visible for the menu. Gameplay hides it while mouse-lock is active.
    setCursorVisible(true, cursorHidden_);
}

InputState Input::poll(bool mouseLock, bool haveImageRect,
                       int imageX, int imageY, int imageW, int imageH,
                       int frameW, int frameH,
                       const InputBindings& b) {
    InputState s;
    setCursorVisible(!mouseLock, cursorHidden_);
    int centerX = imageX + imageW / 2;
    int centerY = imageY + imageH / 2;

    // --- Movement / actions (WASD + modifiers) ---
    s.forward = down(b.forward);
    s.back    = down(b.back);
    s.left    = down(b.left);
    s.right   = down(b.right);
    s.sprint  = down(b.sprint);
    s.jump    = down(b.jump);
    s.crouch  = down(b.crouch);
    s.reload  = down(b.reload);
    s.quit    = down(VK_ESCAPE);

    // Fire: left mouse button OR 'J' key.
    s.mouseLeft = down(VK_LBUTTON);
    s.mouseLeftPressed = s.mouseLeft && !prevMouseLeft_;
    prevMouseLeft_ = s.mouseLeft;

    s.fire = s.mouseLeft || down(b.fireKey);
    s.firePressed = s.fire && !prevFire_;
    prevFire_ = s.fire;

    // --- Keyboard turning fallback (always available) ---
    s.turnLeft  = down(VK_LEFT)  || down('Q');
    s.turnRight = down(VK_RIGHT) || down('E');

    // --- Mouse-look (pointer-lock emulation) ---
    POINT p{};
    bool haveCursor = GetCursorPos(&p) != 0;
    if (haveCursor && haveImageRect && imageW > 0 && imageH > 0) {
        int lx = p.x - imageX;
        int ly = p.y - imageY;
        s.mouseInside = lx >= 0 && ly >= 0 && lx < imageW && ly < imageH;
        // OpenCV may scale the 1920x1080 framebuffer to the actual window image
        // rect. Convert screen-local coordinates back to framebuffer coordinates
        // so UI hit-tests use the same coordinate space as drawing.
        s.mouseX = s.mouseInside ? (int)((double)lx * frameW / imageW) : lx;
        s.mouseY = s.mouseInside ? (int)((double)ly * frameH / imageH) : ly;
    }

    // --- Key edge detection for the settings binding screen ---
    for (int vk : kBindableKeys) {
        bool isDown = down(vk);
        if (isDown && !prevKeys_[vk]) s.keyPressed = vk;
        prevKeys_[vk] = isDown;
    }

    // --- Mouse-look (pointer-lock emulation) ---
    if (mouseLock && haveImageRect) {
        POINT p;
        if (GetCursorPos(&p)) {
            if (warm_) {
                s.mouseDX = static_cast<float>(p.x - centerX);
                s.mouseDY = static_cast<float>(p.y - centerY);
            }
            SetCursorPos(centerX, centerY); // recenter for next frame's delta
            warm_ = true;
        }
    } else {
        warm_ = false; // re-warm when lock resumes to avoid a jump
    }

    return s;
}
