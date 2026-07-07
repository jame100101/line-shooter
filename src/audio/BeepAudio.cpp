#include "audio/BeepAudio.hpp"

#include <algorithm>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

BeepAudio::BeepAudio() { th_ = std::thread(&BeepAudio::worker, this); }

BeepAudio::~BeepAudio() {
    run_ = false;
    cv_.notify_all();
    if (th_.joinable()) th_.join();
}

void BeepAudio::enqueue(int freq, int ms) {
    float v = masterVol_.load() * sfxVol_.load();
    if (v <= 0.01f) return;
    ms = std::max(8, (int)(ms * (0.45f + 0.55f * v)));
    {
        std::lock_guard<std::mutex> lk(m_);
        if (q_.size() > 8) return; // drop if backed up -> keeps audio latency low
        q_.push_back({freq, ms});
    }
    cv_.notify_one();
}

// Short, distinct tones. Low = shot, high ping = hit, bright = kill, thud = hurt.
void BeepAudio::playShot() { enqueue(190,  25); }
void BeepAudio::playHit()  { enqueue(950,  28); }
void BeepAudio::playKill() { enqueue(1500, 90); }
void BeepAudio::playHurt() { enqueue(95, 70); enqueue(70, 90); }

void BeepAudio::setVolume(float master, float sfx) {
    masterVol_ = std::clamp(master, 0.f, 1.f);
    sfxVol_ = std::clamp(sfx, 0.f, 1.f);
}

void BeepAudio::worker() {
    while (run_) {
        Tone t;
        {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [&] { return !q_.empty() || !run_; });
            if (!run_ && q_.empty()) return;
            t = q_.front();
            q_.pop_front();
        }
        Beep(t.freq, t.ms); // blocks THIS thread only; game loop is unaffected
    }
}
