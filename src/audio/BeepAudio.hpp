#pragma once
#include "audio/IAudio.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>

// Placeholder audio backend using the Windows Beep() tone generator on a worker
// thread, so the (blocking) beep never stalls the game loop. Distinct tones make
// shot/hit/kill/hurt audibly different. Swap this for a WAV mixer in a later pass;
// the IAudio contract stays identical.
class BeepAudio : public IAudio {
public:
    BeepAudio();
    ~BeepAudio() override;

    void playShot() override;
    void playHit()  override;
    void playKill() override;
    void playHurt() override;
    void setVolume(float master, float sfx);

private:
    struct Tone { int freq; int ms; };
    void enqueue(int freq, int ms);
    void worker();

    std::thread             th_;
    std::mutex              m_;
    std::condition_variable cv_;
    std::deque<Tone>        q_;
    std::atomic<bool>       run_{true};
    std::atomic<float>      masterVol_{0.75f};
    std::atomic<float>      sfxVol_{0.75f};
};
