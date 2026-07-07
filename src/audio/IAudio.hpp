#pragma once

// Audio interface. Gameplay code calls these; the concrete implementation is
// swappable (a Windows Beep stub now, a real WAV mixer later) without touching
// call sites. Implementations MUST be non-blocking (enqueue + return).
class IAudio {
public:
    virtual ~IAudio() = default;
    virtual void playShot() = 0; // weapon discharge
    virtual void playHit()  = 0; // bullet connected with an enemy
    virtual void playKill() = 0; // enemy killed
    virtual void playHurt() = 0; // player took damage
};
