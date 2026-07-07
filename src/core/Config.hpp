#pragma once
// -----------------------------------------------------------------------------
// Config: ALL tunable gameplay/render parameters live here (single source).
// Every magic number is commented with unit + intent. A JSON loader can later
// overwrite these fields; MVP keeps them in-code for zero-dependency robustness.
// -----------------------------------------------------------------------------
struct Config {
    // ---- Render ----
    int    internalW   = 1600;  // lower than 1920 to improve frame rate
    int    internalH   = 900;   // lower than 1080 to improve frame rate
    double fovDeg      = 120.0; // wide base horizontal FOV (deg)
    double sprintFovDeg = 150.0;// sprint/slide FOV (deg), strong speed distortion
    float  fovLerpSpeed = 10.0f;// FOV smoothing speed (1/s)
    float  viewSmoothTime = 0.12f;  // smooth eye-height transitions (s)
    int    targetFps   = 120;   // frame-rate target; <=0 means uncapped
    bool   lineArtMode = false; // textures are already high-contrast; off for 120fps target

    // World tint colors (BGR, 0..255)
    int ceilB = 248, ceilG = 248, ceilR = 248;  // ceiling
    int floorB = 246, floorG = 246, floorR = 246; // floor

    // ---- Player ----
    float playerRadius = 0.20f; // collision radius (world units, 1 unit = 1 cell)
    int   maxHp        = 100;   // player max HP

    // ---- Movement (world units, seconds) ----
    float walkSpeed    = 4.0f;  // target ground speed (u/s)
    float sprintMult   = 1.7f;  // Shift speed multiplier
    float crouchSpeedMult = 0.45f; // normal crouch movement speed multiplier
    float groundAccel  = 60.0f; // ground acceleration (u/s^2)
    float airAccel     = 14.0f; // air acceleration (enables air-control / bhop)
    float friction     = 10.0f; // ground friction (1/s)
    float jumpImpulse  = 5.2f;  // vertical velocity on jump (u/s)
    float gravity      = 16.0f; // gravity (u/s^2)
    float maxAirSpeed  = 8.5f;  // horizontal speed clamp (prevents runaway bhop)

    // Slide (Ctrl while sprinting & fast)
    float slideBoost    = 1.6f;  // speed multiplier applied at slide start
    float slideFriction = 2.5f;  // low friction during slide (1/s)
    float slideDuration = 0.7f;  // slide length (s)
    float slideMinSpeed = 3.0f;  // min speed required to start a slide (u/s)
    float slideCamDrop  = 125.f; // vertical view translation during slide (px)
    float crouchCamDrop = 90.f;  // vertical view translation during crouch (px)
    float jumpViewScale = 0.13f; // jump height -> screen translation scale

    // ---- Turning ----
    float keyTurnSpeed   = 2.6f;    // arrow/Q-E turn rate (rad/s)
    float mouseSens      = 0.0011f; // yaw   per mouse pixel (rad/px)
    float mousePitchSens = 0.00085f;// pitch per mouse pixel (rad/px)
    float pitchClamp     = 0.75f;   // max |pitch| (rad); yaw remains full 360
    float pitchScreenScale = 0.55f; // reduce horizon warping when looking up/down
    bool  mouseLook      = true;    // default: mouse-lock look (config-switchable)
    bool  invertMouseY   = false;   // false = mouse up looks up (standard FPS)

    // ---- Depth / 3D look ----
    float camHeightFrac  = 0.5f;    // camera height as fraction of screen (floor cast)
    float fogFar         = 46.0f;   // distance (units) at which fog is ~full
    int   fogB = 248, fogG = 248, fogR = 248; // white paper-like distance haze

    // ---- Weapon ----
    float fireInterval    = 0.11f;  // seconds between shots (~9 rounds/s)
    int   magSize         = 30;     // rounds per magazine
    float reloadTime      = 1.10f;  // reload duration (s)
    float weaponRange     = 32.0f;  // hitscan range (world units)
    int   weaponDamage    = 34;     // damage/shot (3 shots kill 100hp)
    float recoilKick      = 0.018f; // upward pitch kick per shot (rad)
    float recoilRecover   = 6.0f;   // pitch recovery rate (1/s)
    float muzzleFlashTime = 0.05f;  // muzzle flash duration (s)
    float aimAssist       = 0.028f; // extra angular hit tolerance (rad)

    // ---- Combat feedback ----
    float hitstopHit  = 0.03f; // time-freeze on hit  (s)
    float hitstopKill = 0.09f; // time-freeze on kill (s)
    float hitTrauma   = 0.08f; // screen-shake trauma on hit  (0..1)
    float killTrauma  = 0.22f; // screen-shake trauma on kill (0..1)
    float shootTrauma = 0.035f;// screen-shake trauma on firing

    // ---- Effects ----
    float traumaDecay  = 1.8f;   // shake trauma decay (1/s)
    float shakeMaxYaw  = 0.010f; // max shake yaw   (rad) at trauma=1
    float shakeMaxPitch= 7.0f;   // max shake pitch (px)  at trauma=1

    // ---- Enemy (Grunt = default archetype) ----
    int   enemyHp          = 100;  // Grunt HP
    float enemySpeed       = 2.2f; // Grunt move speed (u/s)
    float enemyRadius      = 0.30f;// enemy collision/hit radius (u)
    float enemyAttackRange = 1.3f; // melee/attack range (u)
    int   enemyDamage      = 8;    // Grunt damage per attack
    float enemyAttackCd    = 0.9f; // attack cooldown (s)
    float enemyHitFlash    = 0.08f;// white-flash duration on hit (s)
    float enemyStrafeDist  = 4.5f; // begins circling within this range (u)
    float enemyRenderScale = 0.30f;// smaller/lower enemy billboard
    int   gruntColB = 40, gruntColG = 60, gruntColR = 190; // Grunt torso (red, BGR)

    // ---- Rusher archetype (fast, fragile, barely strafes) ----
    int   rusherHp          = 45;   // low HP -> dies fast
    float rusherSpeed       = 4.3f; // faster than the player walk
    float rusherRadius      = 0.26f;
    float rusherAttackRange = 1.2f;
    int   rusherDamage      = 12;   // hits harder to reward killing it early
    float rusherStrafeDist  = 2.0f; // mostly charges straight in
    int   rusherColB = 30, rusherColG = 120, rusherColR = 235; // orange (BGR)
};

// Global read-only accessor (function-local static: no ODR headaches).
inline const Config& cfg() { static Config c; return c; }
