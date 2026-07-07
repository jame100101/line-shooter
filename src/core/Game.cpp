#include "core/Game.hpp"
#include "core/Config.hpp"
#include "core/Math.hpp"
#include "core/Time.hpp"
#include "game/Movement.hpp"
#include "game/Combat.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {
const cv::Scalar kUiRed(36, 48, 224);
const cv::Scalar kUiRedDark(12, 18, 96);

constexpr int kRes[][2] = {
    {1600, 900}, {1920, 1080}, {1280, 960}, {1280, 720}, {1024, 768}
};
constexpr int kFpsOptions[] = {0, 60, 90, 120, 144, 165, 240};

cv::Rect clampRect(const cv::Rect& r, int W, int H) {
    return r & cv::Rect(0, 0, W, H);
}

void blurPanel(cv::Mat& img, cv::Rect r) {
    r = clampRect(r, img.cols, img.rows);
    if (r.empty()) return;
    cv::Mat src = img(r).clone();
    cv::Mat blurred;
    cv::GaussianBlur(src, blurred, cv::Size(0, 0), 3.0);
    cv::Mat paper(src.size(), src.type(), cv::Scalar(238, 238, 238));
    cv::addWeighted(blurred, 0.42, paper, 0.58, 0.0, img(r));
    cv::rectangle(img, r, cv::Scalar(250, 250, 250), 1, cv::LINE_8);
}

void pixelText(cv::Mat& img, const std::string& s, cv::Point org, double scale,
               cv::Scalar color = kUiRed, int thick = 2) {
    cv::putText(img, s, org + cv::Point(3, 3), cv::FONT_HERSHEY_SIMPLEX,
                scale, kUiRedDark, thick + 1, cv::LINE_8);
    cv::putText(img, s, org, cv::FONT_HERSHEY_SIMPLEX,
                scale, color, thick, cv::LINE_8);
}

bool clearLine(const Vec2& a, const Vec2& b, const World& world) {
    Vec2 d = b - a;
    float len = d.length();
    if (len < 1e-4f) return true;
    Vec2 step = d * (0.12f / len);
    int n = static_cast<int>(len / 0.12f);
    Vec2 p = a;
    for (int i = 0; i < n; ++i) {
        p += step;
        if (world.isSolidAt(p.x, p.y)) return false;
    }
    return true;
}
} // namespace

Game::Game()
    : window_("FPS MVP (OpenCV software renderer)", cfg().internalW, cfg().internalH),
      fb_(cfg().internalW, cfg().internalH),
      renderer_(cfg().internalW, cfg().internalH) {
    loadSettings();
    audio_.setVolume(settings_.masterVolume, settings_.sfxVolume);
    applyVideoSettings();
    startRun();
}

void Game::loadSettings() {
    std::ifstream f("settings.cfg");
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        auto asFloat = [&] { return std::stof(v); };
        auto asInt = [&] { return std::stoi(v); };
        try {
            if (k == "mouseSens") settings_.mouseSens = asFloat();
            else if (k == "mousePitchSens") settings_.mousePitchSens = asFloat();
            else if (k == "masterVolume") settings_.masterVolume = clampf(asFloat(), 0.f, 1.f);
            else if (k == "sfxVolume") settings_.sfxVolume = clampf(asFloat(), 0.f, 1.f);
            else if (k == "forward") settings_.bindings.forward = asInt();
            else if (k == "back") settings_.bindings.back = asInt();
            else if (k == "left") settings_.bindings.left = asInt();
            else if (k == "right") settings_.bindings.right = asInt();
            else if (k == "sprint") settings_.bindings.sprint = asInt();
            else if (k == "reload") settings_.bindings.reload = asInt();
            else if (k == "fireKey") settings_.bindings.fireKey = asInt();
            else if (k == "bestKills") bestKills_ = std::max(0, asInt());
            else if (k == "resW") settings_.resW = std::clamp(asInt(), 800, 3840);
            else if (k == "resH") settings_.resH = std::clamp(asInt(), 600, 2160);
            else if (k == "fpsLimit") settings_.fpsLimit = std::clamp(asInt(), 0, 360);
            else if (k == "fullscreen") settings_.fullscreen = asInt() != 0;
        } catch (...) {}
    }
}

void Game::saveSettings() const {
    std::ofstream f("settings.cfg", std::ios::trunc);
    if (!f) return;
    f << "mouseSens=" << settings_.mouseSens << "\n";
    f << "mousePitchSens=" << settings_.mousePitchSens << "\n";
    f << "masterVolume=" << settings_.masterVolume << "\n";
    f << "sfxVolume=" << settings_.sfxVolume << "\n";
    f << "forward=" << settings_.bindings.forward << "\n";
    f << "back=" << settings_.bindings.back << "\n";
    f << "left=" << settings_.bindings.left << "\n";
    f << "right=" << settings_.bindings.right << "\n";
    f << "sprint=" << settings_.bindings.sprint << "\n";
    f << "reload=" << settings_.bindings.reload << "\n";
    f << "fireKey=" << settings_.bindings.fireKey << "\n";
    f << "bestKills=" << bestKills_ << "\n";
    f << "resW=" << settings_.resW << "\n";
    f << "resH=" << settings_.resH << "\n";
    f << "fpsLimit=" << settings_.fpsLimit << "\n";
    f << "fullscreen=" << (settings_.fullscreen ? 1 : 0) << "\n";
}

void Game::applyVideoSettings() {
    settings_.resW = std::clamp(settings_.resW, 800, 3840);
    settings_.resH = std::clamp(settings_.resH, 600, 2160);
    fb_.resize(settings_.resW, settings_.resH);
    renderer_ = Renderer(settings_.resW, settings_.resH);
    window_.resize(settings_.resW, settings_.resH);
    window_.setFullscreen(settings_.fullscreen);
}

const char* Game::resolutionLabel() const {
    static char buf[32];
    std::snprintf(buf, sizeof buf, "%d x %d", settings_.resW, settings_.resH);
    return buf;
}

const char* Game::fpsLabel() const {
    static char buf[24];
    if (settings_.fpsLimit <= 0) return "UNCAPPED";
    std::snprintf(buf, sizeof buf, "%d FPS", settings_.fpsLimit);
    return buf;
}

void Game::startRun() {
    player_.pos = world_.playerSpawn();
    player_.angle = 0.f;
    player_.pitch = 0.f;
    player_.viewFovDeg = (float)cfg().fovDeg;
    player_.hp = cfg().maxHp;
    player_.vel = Vec2{0, 0};
    player_.z = player_.vz = 0.f;
    player_.camDrop = player_.viewShiftPx = player_.viewShiftVel = 0.f;
    player_.grounded = true;
    player_.sprinting = player_.sliding = player_.crouching = false;

    weapon_ = Weapon{};
    weapon_.ammo = weapon_.magSize = cfg().magSize;
    effects_ = Effects{};
    hud_ = Hud{};
    kills_ = 0;
    hurtFlash_ = 0.f;
    spawnEnemies();
}

void Game::commitBestKills() {
    if (kills_ > bestKills_) {
        bestKills_ = kills_;
        saveSettings();
    }
}

void Game::spawnEnemies() {
    enemies_.clear();
    const auto& spawns = world_.enemySpawns();
    const int initial = std::min<int>(5, (int)spawns.size());
    for (int i = 0; i < initial; ++i) {
        EnemyType t = (i % 3 == 1) ? EnemyType::Rusher : EnemyType::Grunt;
        enemies_.push_back(spawnEnemy(t, spawns[(size_t)i]));
    }
    enemySpawnCursor_ = initial;
    enemySpawnTimer_ = 0.f;
}

void Game::maintainEnemyPopulation(float dt) {
    const int maxAlive = 5;
    int alive = 0;
    for (const auto& e : enemies_) if (e.alive) ++alive;
    if (alive >= maxAlive) { enemySpawnTimer_ = 0.f; return; }

    enemySpawnTimer_ += dt;
    if (enemySpawnTimer_ < 2.0f) return;
    enemySpawnTimer_ = 0.f;

    const auto& spawns = world_.enemySpawns();
    if (spawns.empty()) return;

    int chosen = -1;
    const float minSpawnDist2 = 11.0f * 11.0f;
    for (int tries = 0; tries < (int)spawns.size(); ++tries) {
        int idx = (enemySpawnCursor_ + tries) % (int)spawns.size();
        Vec2 p = spawns[(size_t)idx];
        if ((p - player_.pos).length2() < minSpawnDist2) continue;
        // Prefer spawn points that are not directly visible from the player.
        if (clearLine(player_.pos, p, world_)) continue;
        chosen = idx;
        enemySpawnCursor_ = idx + 1;
        break;
    }
    if (chosen < 0) {
        for (int tries = 0; tries < (int)spawns.size(); ++tries) {
            int idx = (enemySpawnCursor_ + tries) % (int)spawns.size();
            if ((spawns[(size_t)idx] - player_.pos).length2() >= minSpawnDist2) {
                chosen = idx;
                enemySpawnCursor_ = idx + 1;
                break;
            }
        }
    }
    if (chosen < 0) { enemySpawnTimer_ = 1.4f; return; } // delay, never pop next to player

    EnemyType t = (chosen % 3 == 1) ? EnemyType::Rusher : EnemyType::Grunt;
    enemies_.push_back(spawnEnemy(t, spawns[(size_t)chosen]));

    // Keep the vector bounded; dead enemies no longer need to be retained.
    if (enemies_.size() > 16) {
        enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                      [](const Enemy& e) { return !e.alive; }),
                       enemies_.end());
    }
}

void Game::handleShooting(float /*gdt*/, const InputState& in) {
    const Config& c = cfg();
    if (in.reload) weapon_.reload();

    // Full-auto: holding fire keeps shooting, gated by the weapon cooldown.
    if (in.fire && weapon_.tryFire()) {
        ShotTrace trace = Combat::traceShot(player_, enemies_, world_);
        Vec2 dir{std::cos(player_.angle), std::sin(player_.angle)};
        Vec2 right{-std::sin(player_.angle), std::cos(player_.angle)};
        // Spatial muzzle point captured at the instant of firing. The line will
        // remain in world space even when the player turns afterward.
        Vec2 muzzleWorld = player_.pos + dir * 0.72f + right * 0.28f;
        float fovDeg = player_.viewFovDeg > 1.f ? player_.viewFovDeg : (float)c.fovDeg;
        float aspect = (float)fb_.width() / (float)fb_.height();
        float baseVFov = 2.f * std::atan(std::tan(deg2rad((float)c.fovDeg) * 0.5f) / aspect);
        float curVFov  = 2.f * std::atan(std::tan(deg2rad(fovDeg) * 0.5f) / aspect);
        float verticalScale = std::tan(baseVFov * 0.5f) / std::tan(curVFov * 0.5f);
        verticalScale = clampf(verticalScale, 0.45f, 1.15f);
        float pitchPx = (player_.pitch * c.pitchScreenScale + weapon_.recoil) *
                        (fb_.height() / deg2rad(fovDeg));
        float endDepth = std::max(0.05f, (trace.end - player_.pos).length());
        float startDepth = std::max(0.05f, dot(muzzleWorld - player_.pos, dir));
        float endZ = pitchPx * endDepth / (fb_.height() * verticalScale);
        float muzzleScreenY = fb_.height() * 0.612f;
        float startZ = (fb_.height() * 0.5f + pitchPx - muzzleScreenY) *
                       startDepth / (fb_.height() * verticalScale);
        effects_.spawnTracer(muzzleWorld, trace.end, startZ, endZ);
        effects_.addTrauma(c.shootTrauma);              // recoil shake
        audio_.playShot();
        ShotResult r = Combat::resolveShot(player_, enemies_, world_,
                                           pitchPx, (float)fb_.height(), verticalScale);
        if (r.hit) {
            hud_.onHit();
            audio_.playHit();
            effects_.addTrauma(c.hitTrauma);
            effects_.addHitstop(c.hitstopHit);
        }
        if (r.kill) {
            ++kills_;
            hud_.onKill();
            audio_.playKill();
            effects_.addTrauma(c.killTrauma);
            effects_.addHitstop(c.hitstopKill);
            // Gib burst at screen center (where the crosshair/kill happened).
            effects_.spawnBurst(fb_.width() * 0.5f, fb_.height() * 0.5f,
                                24, 40, 60, 200);
        }
    }
}

void Game::update(float dt, const InputState& in) {
    const Config& c = cfg();

    // ---- Mouse-look (yaw + pitch); keyboard turn handled inside Movement ----
    if (c.mouseLook) {
        // Horizontal: mouse right (+dx) turns right. Vertical: mouse up (-dy)
        // looks up by default; invertMouseY flips it. Both axes now consistent.
        float dpitch = in.mouseDY * settings_.mousePitchSens;
        if (!c.invertMouseY) dpitch = -dpitch;      // mouse up -> look up
        player_.angle = wrapAngle(player_.angle + in.mouseDX * settings_.mouseSens);
        player_.pitch = clampf(player_.pitch + dpitch, -c.pitchClamp, c.pitchClamp);
    }

    // ---- Hit-stop: freeze gameplay briefly for punch, but keep juice alive ----
    float gdt = (effects_.hitstop() > 0.f) ? 0.f : dt;

    if (gdt > 0.f) {
        Movement::update(player_, in, world_, gdt);
        for (auto& e : enemies_) {
            int dmg = EnemyAI::update(e, player_, world_, gdt);
            if (dmg > 0) {
                player_.hp -= dmg;
                hurtFlash_ = std::min(0.55f, hurtFlash_ + 0.32f);
                effects_.addTrauma(0.12f);
                audio_.playHurt();
            }
        }
        maintainEnemyPopulation(gdt);
    }
    weapon_.update(gdt);
    handleShooting(gdt, in);

    // Effects/particles always advance in real time (shake settles during freeze).
    effects_.update(dt);
    hud_.update(dt);
    hurtFlash_ = std::max(0.f, hurtFlash_ - dt * 1.9f);

    if (player_.hp <= 0) {
        player_.hp = 0;
        commitBestKills();
        screen_ = Screen::GameOver;
    }
}

void Game::render(float fps) {
    fb_.clear(0, 0, 0);
    renderer_.renderWorldAndActors(fb_, world_, player_, enemies_, weapon_,
                                   effects_, assets_);
    hud_.draw(fb_, player_, weapon_, kills_, fps);
    if (hurtFlash_ > 0.f) {
        cv::Mat overlay(fb_.height(), fb_.width(), CV_8UC3, cv::Scalar(18, 18, 210));
        cv::addWeighted(overlay, clampf(hurtFlash_, 0.f, 0.45f), fb_.mat(),
                        1.f - clampf(hurtFlash_, 0.f, 0.45f), 0.0, fb_.mat());
        int inset = 18 + (int)(hurtFlash_ * 30.f);
        cv::rectangle(fb_.mat(), cv::Rect(inset, inset,
                      fb_.width() - inset * 2, fb_.height() - inset * 2),
                      kUiRed, 4, cv::LINE_8);
    }
    window_.show(fb_.mat());
}

bool Game::hit(const InputState& in, const cv::Rect& r) const {
    return in.mouseInside && r.contains(cv::Point(in.mouseX, in.mouseY));
}

void Game::drawButton(const cv::Rect& r, const char* label, bool hover) {
    cv::Mat& img = fb_.mat();
    blurPanel(img, r);
    cv::Scalar ink = hover ? kUiRed : cv::Scalar(80, 80, 80);
    cv::rectangle(img, r, ink, hover ? 3 : 2, cv::LINE_8);
    int base = 0;
    cv::Size ts = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.82, 2, &base);
    pixelText(img, label, {r.x + (r.width - ts.width) / 2, r.y + (r.height + ts.height) / 2},
              0.82, kUiRed, 2);
}

void Game::drawSlider(const char* label, cv::Rect track, float value) {
    cv::Mat& img = fb_.mat();
    pixelText(img, label, {track.x, track.y - 18}, 0.55, kUiRed, 1);
    cv::rectangle(img, track, cv::Scalar(238, 238, 238), cv::FILLED);
    cv::rectangle(img, track, cv::Scalar(82, 82, 82), 1, cv::LINE_8);
    int knobX = track.x + (int)(std::clamp(value, 0.f, 1.f) * track.width);
    cv::rectangle(img, cv::Rect(knobX - 7, track.y - 8, 14, track.height + 16), kUiRed, cv::FILLED);
}

const char* Game::keyName(int vk) const {
    switch (vk) {
        case 0x10: return "SHIFT"; case 0x11: return "CTRL"; case 0x12: return "ALT";
        case 0x20: return "SPACE"; case 0x09: return "TAB";
        case 0x25: return "LEFT"; case 0x26: return "UP"; case 0x27: return "RIGHT"; case 0x28: return "DOWN";
        case 0x01: return "MOUSE1"; case 0x02: return "MOUSE2";
        default: break;
    }
    static char buf[8];
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        buf[0] = (char)vk; buf[1] = 0; return buf;
    }
    std::snprintf(buf, sizeof buf, "%02X", vk);
    return buf;
}

void Game::updateMenu(const InputState& in) {
    cv::Rect play(fb_.width()/2 - 180, fb_.height()/2 - 120, 360, 64);
    cv::Rect settings(fb_.width()/2 - 180, fb_.height()/2 - 40, 360, 64);
    cv::Rect exit(fb_.width()/2 - 180, fb_.height()/2 + 40, 360, 64);
    if (in.mouseLeftPressed && hit(in, play)) { startRun(); screen_ = Screen::Playing; }
    if (in.mouseLeftPressed && hit(in, settings)) screen_ = Screen::Settings;
    if (in.mouseLeftPressed && hit(in, exit)) running_ = false;
    if (in.keyPressed == 0x20) { startRun(); screen_ = Screen::Playing; }
}

void Game::updateGameOver(const InputState& in) {
    cv::Rect retry(fb_.width()/2 - 300, fb_.height()/2 + 80, 260, 68);
    cv::Rect menu(fb_.width()/2 + 40, fb_.height()/2 + 80, 320, 68);
    if (in.quit) { screen_ = Screen::MainMenu; return; }
    if (in.mouseLeftPressed && hit(in, retry)) { startRun(); screen_ = Screen::Playing; }
    if (in.mouseLeftPressed && hit(in, menu)) { screen_ = Screen::MainMenu; }
}

void Game::updateSettings(const InputState& in) {
    if (in.quit) { bindingTarget_ = -1; saveSettings(); screen_ = Screen::MainMenu; return; }

    int* bindSlots[] = {
        &settings_.bindings.forward, &settings_.bindings.back, &settings_.bindings.left,
        &settings_.bindings.right, &settings_.bindings.sprint,
        &settings_.bindings.reload, &settings_.bindings.fireKey
    };
    if (bindingTarget_ >= 0) {
        if (in.keyPressed) {
            *bindSlots[bindingTarget_] = in.keyPressed;
            bindingTarget_ = -1;
        }
        return;
    }

    auto sliderHit = [&](cv::Rect r, float& v) {
        if (in.mouseLeft && hit(in, r)) {
            v = clampf((float)(in.mouseX - r.x) / r.width, 0.f, 1.f);
            return true;
        }
        return false;
    };

    cv::Rect sens(420, 210, 540, 16), vol(420, 315, 540, 16), sfx(420, 420, 540, 16);
    float sensNorm = clampf((settings_.mouseSens - 0.00035f) / (0.0020f - 0.00035f), 0.f, 1.f);
    if (sliderHit(sens, sensNorm)) {
        settings_.mouseSens = lerpf(0.00035f, 0.0020f, sensNorm);
        settings_.mousePitchSens = settings_.mouseSens * 0.78f;
    }
    sliderHit(vol, settings_.masterVolume);
    sliderHit(sfx, settings_.sfxVolume);
    audio_.setVolume(settings_.masterVolume, settings_.sfxVolume);

    cv::Rect resBtn(420, 520, 230, 46);
    cv::Rect fullBtn(690, 520, 230, 46);
    cv::Rect fpsBtn(420, 585, 230, 46);
    if (in.mouseLeftPressed && hit(in, resBtn)) {
        int cur = 0;
        for (int i = 0; i < (int)(sizeof(kRes) / sizeof(kRes[0])); ++i)
            if (settings_.resW == kRes[i][0] && settings_.resH == kRes[i][1]) cur = i;
        cur = (cur + 1) % (int)(sizeof(kRes) / sizeof(kRes[0]));
        settings_.resW = kRes[cur][0];
        settings_.resH = kRes[cur][1];
        applyVideoSettings();
        return;
    }
    if (in.mouseLeftPressed && hit(in, fullBtn)) {
        settings_.fullscreen = !settings_.fullscreen;
        applyVideoSettings();
        return;
    }
    if (in.mouseLeftPressed && hit(in, fpsBtn)) {
        int n = (int)(sizeof(kFpsOptions) / sizeof(kFpsOptions[0]));
        int cur = 0;
        for (int i = 0; i < n; ++i)
            if (settings_.fpsLimit == kFpsOptions[i]) cur = i;
        settings_.fpsLimit = kFpsOptions[(cur + 1) % n];
    }

    cv::Rect back(70, fb_.height() - 110, 220, 60);
    cv::Rect save(320, fb_.height() - 110, 220, 60);
    if (in.mouseLeftPressed && hit(in, back)) { screen_ = Screen::MainMenu; }
    if (in.mouseLeftPressed && hit(in, save)) { applyVideoSettings(); saveSettings(); return; }

    int labelX = std::max(720, fb_.width() - 520);
    int buttonX = std::max(980, fb_.width() - 280);
    for (int i = 0; i < 7; ++i) {
        cv::Rect br(buttonX, 205 + i * 58, 220, 42);
        if (in.mouseLeftPressed && hit(in, br)) bindingTarget_ = i;
    }
}

void Game::renderMainMenu() {
    fb_.clear(248, 248, 248);
    cv::Mat& img = fb_.mat();
    cv::Scalar ink(22, 22, 22);
    for (int x = -fb_.height(); x < fb_.width(); x += 46)
        cv::line(img, {x, 0}, {x + fb_.height(), fb_.height()}, cv::Scalar(26,26,26), 1, cv::LINE_AA);
    for (int y = 0; y < fb_.height(); y += 90)
        cv::line(img, {0, y}, {fb_.width(), y}, cv::Scalar(210,210,210), 1, cv::LINE_8);
    pixelText(img, "LINE FPS", {fb_.width()/2 - 220, 250}, 1.8, kUiRed, 4);
    cv::Rect play(fb_.width()/2 - 180, fb_.height()/2 - 120, 360, 64);
    cv::Rect settings(fb_.width()/2 - 180, fb_.height()/2 - 40, 360, 64);
    cv::Rect exit(fb_.width()/2 - 180, fb_.height()/2 + 40, 360, 64);
    drawButton(play, "PLAY", false);
    drawButton(settings, "SETTING", false);
    drawButton(exit, "EXIT", false);
    char best[64]; std::snprintf(best, sizeof best, "BEST KILL: %d", bestKills_);
    blurPanel(img, cv::Rect(fb_.width()/2 - 200, fb_.height()/2 + 122, 400, 56));
    pixelText(img, best, {fb_.width()/2 - 165, fb_.height()/2 + 162}, 0.85, kUiRed, 2);
    pixelText(img, "SPACE: PLAY   ESC: QUIT", {fb_.width()/2 - 250, fb_.height() - 105},
              0.55, kUiRed, 1);
    window_.show(img);
}

void Game::renderSettings() {
    fb_.clear(248, 248, 248);
    cv::Mat& img = fb_.mat();
    cv::Scalar ink(45, 45, 45);
    pixelText(img, "SETTING", {70, 105}, 1.15, kUiRed, 3);

    cv::Rect sens(420, 210, 540, 16), vol(420, 315, 540, 16), sfx(420, 420, 540, 16);
    float sensNorm = clampf((settings_.mouseSens - 0.00035f) / (0.0020f - 0.00035f), 0.f, 1.f);
    drawSlider("Mouse Sensitivity", sens, sensNorm);
    drawSlider("Master Volume", vol, settings_.masterVolume);
    drawSlider("SFX Volume", sfx, settings_.sfxVolume);

    pixelText(img, "Video", {420, 500}, 0.58, kUiRed, 1);
    drawButton({420, 520, 230, 46}, resolutionLabel(), false);
    drawButton({690, 520, 230, 46}, settings_.fullscreen ? "FULLSCREEN" : "WINDOWED", false);
    drawButton({420, 585, 230, 46}, fpsLabel(), false);

    const char* names[] = {"Forward","Back","Left","Right","Sprint","Reload","Fire Key"};
    int vals[] = {settings_.bindings.forward, settings_.bindings.back, settings_.bindings.left,
                  settings_.bindings.right, settings_.bindings.sprint,
                  settings_.bindings.reload, settings_.bindings.fireKey};
    int labelX = std::max(720, fb_.width() - 520);
    int buttonX = std::max(980, fb_.width() - 280);
    pixelText(img, "Key Bindings", {labelX, 145}, 0.65, kUiRed, 2);
    for (int i = 0; i < 7; ++i) {
        int y = 205 + i * 58;
        pixelText(img, names[i], {labelX, y + 29}, 0.44, kUiRed, 1);
        cv::Rect br(buttonX, y, 220, 42);
        const char* txt = (bindingTarget_ == i) ? "PRESS KEY..." : keyName(vals[i]);
        drawButton(br, txt, bindingTarget_ == i);
    }

    drawButton({70, fb_.height() - 110, 220, 60}, "BACK", false);
    drawButton({320, fb_.height() - 110, 220, 60}, "SAVE", false);
    pixelText(img, "Click binding, press key, then SAVE. Esc returns to menu.",
              {70, fb_.height() - 35}, 0.46, kUiRed, 1);
    window_.show(img);
}

void Game::renderGameOver(float fps) {
    fb_.clear(0, 0, 0);
    renderer_.renderWorldAndActors(fb_, world_, player_, enemies_, weapon_,
                                   effects_, assets_);
    hud_.draw(fb_, player_, weapon_, kills_, fps);

    cv::Mat& img = fb_.mat();
    const int W = fb_.width(), H = fb_.height();
    cv::Rect panel(W/2 - 430, H/2 - 210, 860, 380);
    blurPanel(img, panel);
    cv::rectangle(img, panel, kUiRed, 3, cv::LINE_8);

    pixelText(img, "GAME OVER", {W/2 - 255, H/2 - 80}, 1.55, kUiRed, 4);
    char score[96]; std::snprintf(score, sizeof score, "KILL: %d    BEST: %d", kills_, bestKills_);
    pixelText(img, score, {W/2 - 250, H/2 - 18}, 0.80, kUiRed, 2);

    cv::Rect retry(W/2 - 300, H/2 + 80, 260, 68);
    cv::Rect menu(W/2 + 40, H/2 + 80, 320, 68);
    drawButton(retry, "RETRY", false);
    drawButton(menu, "RETURN MENU", false);
    window_.show(img);
}

void Game::run() {
    const Config& c = cfg();
    auto prev = timeutil::now();
    float fpsSmooth = settings_.fpsLimit > 0 ? (float)settings_.fpsLimit : 120.f;

    // Headless self-test: FPS_AUTOFRAMES=N renders N frames, writes shot.png, and
    // exits (no mouse-lock). Used to verify the software renderer without a human.
    int autoFrames = 0;
    if (const char* af = std::getenv("FPS_AUTOFRAMES")) autoFrames = std::atoi(af);
    if (autoFrames > 0) screen_ = Screen::Playing;
    int frame = 0;

    while (running_) {
        auto frameStart = timeutil::now();
        double dt = timeutil::seconds(prev, frameStart);
        prev = frameStart;
        if (dt > 0.05) dt = 0.05; // clamp huge steps (e.g. after a stall)

        cv::Rect imageRect;
        bool haveImageRect = window_.imageRect(imageRect);
        bool useMouse = c.mouseLook && (autoFrames == 0) && (screen_ == Screen::Playing);
        InputState in = input_.poll(useMouse, haveImageRect,
                                    imageRect.x, imageRect.y, imageRect.width, imageRect.height,
                                    fb_.width(), fb_.height(),
                                    settings_.bindings);

        // In headless self-test, hold the trigger so combat/feedback is exercised
        // (spawn faces an enemy) and the captured frame shows muzzle + hit marker.
        if (autoFrames > 0) in.fire = true;

        if (autoFrames == 0 && !window_.isOpen()) { running_ = false; break; }

        if (screen_ == Screen::MainMenu) {
            if (in.quit) { running_ = false; break; }
            updateMenu(in);
            renderMainMenu();
            window_.pump(0);
        } else if (screen_ == Screen::Settings) {
            updateSettings(in);
            renderSettings();
            window_.pump(0);
        } else if (screen_ == Screen::GameOver) {
            updateGameOver(in);
            if (screen_ == Screen::GameOver) renderGameOver(fpsSmooth);
            else if (screen_ == Screen::MainMenu) renderMainMenu();
            else render(fpsSmooth);
            window_.pump(0);
        } else {
            if (autoFrames == 0 && in.quit) {
                commitBestKills();
                screen_ = Screen::MainMenu;
                window_.pump(0);
                continue;
            }
            update((float)dt, in);
            render(fpsSmooth);
            window_.pump(0); // keep OS window responsive + present
        }

        if (autoFrames > 0 && ++frame >= autoFrames) {
            cv::imwrite("shot.png", fb_.mat()); // capture software-rendered frame
            std::printf("[auto] final smoothed FPS = %.1f\n", fpsSmooth);
            running_ = false;
            break;
        }

        // ---- Optional frame cap. targetFps <= 0 intentionally runs uncapped. ----
        int fpsLimit = settings_.fpsLimit;
        bool capFrames = fpsLimit > 0;
        double targetFrame = capFrames ? (1.0 / fpsLimit) : 0.0;
        if (capFrames) {
            double elapsed = timeutil::seconds(frameStart, timeutil::now());
            double remain = targetFrame - elapsed;
            if (remain > 0.002)
                timeutil::sleepSeconds(remain - 0.001); // coarse sleep
            while (timeutil::seconds(frameStart, timeutil::now()) < targetFrame) {
                // short spin trims std::this_thread::sleep_for overshoot at 120 Hz
            }
        }

        double total = timeutil::seconds(frameStart, timeutil::now());
        if (total > 1e-6) // exponential smoothing of the FPS readout
            fpsSmooth = lerpf(fpsSmooth, (float)(1.0 / total), 0.1f);
    }
}
