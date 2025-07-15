// CHIP8CHAPA - CHIP-8 Sound implementation
// Handles beeper, XO-CHIP pattern playback, and audio output via SDL2

#include "chip8_sound.h"
#include <cmath>
#include <array>
#include <atomic>
#include <cstring>

constexpr int CHIP8_SAMPLE_RATE = 44100;
constexpr int CHIP8_BEEP_FREQ = 440;
constexpr int CHIP8_AMPLITUDE = 64;

constexpr int XOCHIP_PATTERN_BITS = 128;
constexpr int XOCHIP_PATTERN_BYTES = 16;
constexpr int XOCHIP_PATTERN_RATE = 4000;

namespace {
    inline bool get_pattern_bit(const uint8_t* pattern, int bit) {
        return (pattern[bit / 8] >> (7 - (bit % 8))) & 1;
    }
}

std::array<uint8_t, XOCHIP_PATTERN_BYTES> patternBuffer = {};
std::atomic<bool> patternPlaying = false;
std::atomic<int> patternBit = 0;

Chip8Sound::Chip8Sound() : buzzerOn(false), audioDevice(0), playing(false), phase(0) {
    SDL_AudioSpec want{};
    want.freq = CHIP8_SAMPLE_RATE;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 512;
    want.callback = audioCallback;
    want.userdata = this;
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
    if (audioDevice != 0) {
        SDL_PauseAudioDevice(audioDevice, 0);
    }
}

Chip8Sound::~Chip8Sound() {
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
    }
}

void Chip8Sound::start() {
    buzzerOn.store(true, std::memory_order_relaxed);
    playing = true;
}

void Chip8Sound::stop() {
    buzzerOn.store(false, std::memory_order_relaxed);
    playing = false;
    patternPlaying = false;
    patternBit = 0;
}

bool Chip8Sound::isOn() const {
    return buzzerOn.load(std::memory_order_relaxed);
}

void Chip8Sound::update() {
    playing = buzzerOn.load(std::memory_order_relaxed);
}

void Chip8Sound::playPattern(const uint8_t* pattern) {
    for (int i = 0; i < XOCHIP_PATTERN_BYTES; ++i) {
        patternBuffer[i] = pattern[i];
    }
    patternPlaying = true;
    patternBit = 0;
}

void Chip8Sound::forceSilence() {
    if (audioDevice != 0) {
        SDL_PauseAudioDevice(audioDevice, 1);
        SDL_PauseAudioDevice(audioDevice, 0);
    }
}

int Chip8Sound::getPhase() const { return phase; }
void Chip8Sound::setPhase(int p) { phase = p; }
bool Chip8Sound::getMuted() const { return muted; }
void Chip8Sound::setMuted(bool m) { muted = m; }
int Chip8Sound::getVolume() const { return volume; }
void Chip8Sound::setVolume(int v) { volume = v; }
bool Chip8Sound::getPlaying() const { return playing; }
void Chip8Sound::setPlaying(bool p) { playing = p; }

void Chip8Sound::playTestBeep() {
    start();
    SDL_Delay(100);
    stop();
}

void Chip8Sound::audioCallback(void* userdata, Uint8* stream, int len) {
    Chip8Sound* self = static_cast<Chip8Sound*>(userdata);
    if (self->muted) {
        memset(stream, 128, len);
        return;
    }
    if (!self->playing && !patternPlaying) {
        memset(stream, 128, len);
        return;
    }
    int vol = self->volume;
    for (int i = 0; i < len; ++i) {
        if (patternPlaying) {
            int samplesPerBit = CHIP8_SAMPLE_RATE / XOCHIP_PATTERN_RATE;
            int bitIdx = patternBit / samplesPerBit;
            if (bitIdx < XOCHIP_PATTERN_BITS) {
                int base = get_pattern_bit(patternBuffer.data(), bitIdx) ? (128 + CHIP8_AMPLITUDE) : (128 - CHIP8_AMPLITUDE);
                stream[i] = 128 + ((base - 128) * vol) / 100;
                ++patternBit;
            } else {
                stream[i] = 128;
                patternPlaying = false;
                patternBit = 0;
            }
            continue;
        }
        if (self->playing) {
            int period = CHIP8_SAMPLE_RATE / CHIP8_BEEP_FREQ;
            int base = ((self->phase < period / 2) ? (128 + CHIP8_AMPLITUDE) : (128 - CHIP8_AMPLITUDE));
            stream[i] = 128 + ((base - 128) * vol) / 100;
            self->phase = (self->phase + 1) % period;
        } else {
            stream[i] = 128;
        }
    }
} 