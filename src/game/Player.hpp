#pragma once
#include "core/Math.hpp"

// Player state: kinematics + movement flags. Pure data; logic lives in Movement.
struct Player {
    Vec2  pos;                 // world position (units)
    float angle = 0.f;         // yaw (rad), 0 = +x
    float pitch = 0.f;         // vertical look/recoil (rad); +up (looks upward)

    Vec2  vel;                 // horizontal velocity (u/s)
    float z  = 0.f;            // height above ground (units) for jump
    float vz = 0.f;            // vertical velocity (u/s)

    bool  grounded  = true;    // on the floor?
    bool  sliding   = false;   // currently sliding?
    bool  crouching = false;   // crouch held without sliding?
    bool  sprinting = false;   // sprint active this frame?
    float slideTimer = 0.f;    // remaining slide time (s)
    float camDrop    = 0.f;    // deprecated/unused legacy field
    float viewShiftPx = 0.f;   // final smoothed jump/crouch screen translation
    float viewShiftVel = 0.f;  // velocity for smooth-damp eye-height motion
    float viewFovDeg = 66.f;   // smoothed horizontal FOV (deg)

    int   hp = 100;            // current HP
};
