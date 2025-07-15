// CHIP8CHAPA - CHIP-8 Sound header
// Declares beeper, XO-CHIP pattern playback, and audio output API

#ifndef CHIP8_SOUND_H
#define CHIP8_SOUND_H

#include <SDL.h>
#include <atomic>

class Chip8Sound {
public:
    Chip8Sound();
    ~Chip8Sound();
    Chip8Sound(const Chip8Sound&) = delete;
    Chip8Sound& operator=(const Chip8Sound&) = delete;
    Chip8Sound(Chip8Sound&&) = delete;
    Chip8Sound& operator=(Chip8Sound&&) = delete;

    // Start/stop the buzzer
    void start();
    void stop();
    // Query if the buzzer is currently on
    bool isOn() const;
    // Call this periodically to update audio state
    void update();
    // Immediately silence audio output (flushes buffer)
    void forceSilence();

    // Play a XO-CHIP audio pattern (16 bytes, 128 bits, 1-bit PCM at 4000Hz)
    void playPattern(const uint8_t* pattern);
    void setMuted(bool muted);
    void setVolume(int percent); // 0-100
    void playTestBeep();

    int getPhase() const;
    void setPhase(int);
    bool getMuted() const;
    int getVolume() const;
    bool getPlaying() const;
    void setPlaying(bool);

private:
    std::atomic<bool> buzzerOn;
    SDL_AudioDeviceID audioDevice;
    static void audioCallback(void* userdata, Uint8* stream, int len);
    std::atomic<bool> playing;
    int phase;
    bool muted = false;
    int volume = 100;
};

#endif