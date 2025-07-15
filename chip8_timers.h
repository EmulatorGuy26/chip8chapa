// CHIP8CHAPA - CHIP-8 Timers header
// Declares delay and sound timer API for CHIP-8

#ifndef CHIP8_TIMERS_H
#define CHIP8_TIMERS_H

#include <cstdint>

class Chip8Timers {
public:
    Chip8Timers();

    // Set/get delay timer (ticks down at 60Hz)
    void setDelay(uint8_t value);
    uint8_t getDelay() const;

    // Set/get sound timer (ticks down at 60Hz)
    void setSound(uint8_t value);
    uint8_t getSound() const;

    // Call this at 60Hz to decrement timers
    void tick();

private:
    uint8_t delay;
    uint8_t sound;
};

#endif