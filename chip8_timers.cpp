// CHIP8CHAPA - CHIP-8 Timers implementation
// Handles delay and sound timers for CHIP-8

#include "chip8_timers.h"

Chip8Timers::Chip8Timers() : delay(0), sound(0) {}

void Chip8Timers::setDelay(uint8_t value) {
    delay = value;
}

void Chip8Timers::setSound(uint8_t value) {
    sound = value;
}

uint8_t Chip8Timers::getDelay() const {
    return delay;
}

uint8_t Chip8Timers::getSound() const {
    return sound;
}

void Chip8Timers::tick() {
    if (delay > 0) --delay;
    if (sound > 0) --sound;
} 