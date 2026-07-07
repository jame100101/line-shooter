#pragma once
#include <vector>
#include "platform/Window.hpp"
#include "platform/Input.hpp"
#include "render/Framebuffer.hpp"
#include "render/Renderer.hpp"
#include "assets/AssetManager.hpp"
#include "game/World.hpp"
#include "game/Player.hpp"
#include "game/Weapon.hpp"
#include "game/Enemy.hpp"
#include "game/Effects.hpp"
#include "ui/Hud.hpp"
#include "audio/BeepAudio.hpp"

// Game owns all subsystems and runs the fixed-cap main loop:
// input -> gameplay update (with hit-stop) -> render -> HUD -> present -> cap.
class Game {
public:
    Game();
    void run();

private:
    enum class Screen { MainMenu, Settings, Playing, GameOver };

    struct RuntimeSettings {
        float mouseSens = 0.0011f;
        float mousePitchSens = 0.00085f;
        float masterVolume = 0.75f;
        float sfxVolume = 0.75f;
        int resW = 1600;
        int resH = 900;
        int fpsLimit = 120; // 0 = uncapped
        bool fullscreen = false;
        InputBindings bindings;
    };

    void spawnEnemies();
    void maintainEnemyPopulation(float dt);
    void startRun();
    void commitBestKills();
    void applyVideoSettings();
    const char* resolutionLabel() const;
    const char* fpsLabel() const;
    void update(float dt, const InputState& in);
    void render(float fps);
    void handleShooting(float gdt, const InputState& in);
    void updateMenu(const InputState& in);
    void updateSettings(const InputState& in);
    void updateGameOver(const InputState& in);
    void renderMainMenu();
    void renderSettings();
    void renderGameOver(float fps);
    void loadSettings();
    void saveSettings() const;
    void drawButton(const cv::Rect& r, const char* label, bool hover);
    void drawSlider(const char* label, cv::Rect track, float value);
    bool hit(const InputState& in, const cv::Rect& r) const;
    const char* keyName(int vk) const;

    Window       window_;
    Input        input_;
    Framebuffer  fb_;
    AssetManager assets_;
    World        world_;
    Player       player_;
    Weapon       weapon_;
    std::vector<Enemy> enemies_;
    Effects      effects_;
    Hud          hud_;
    BeepAudio    audio_;
    Renderer     renderer_;

    int  kills_   = 0;
    int  bestKills_ = 0;
    float hurtFlash_ = 0.f;
    bool running_ = true;
    Screen screen_ = Screen::MainMenu;
    RuntimeSettings settings_;
    int bindingTarget_ = -1; // 0..N waiting for new key, -1 idle
    float enemySpawnTimer_ = 0.f;
    int enemySpawnCursor_ = 0;
};
