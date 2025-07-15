// CHIP8CHAPA - CHIP-8 Input implementation
// Handles key state for CHIP-8 keypad (0-F)

#include "chip8_input.h"

Chip8Input::Chip8Input() {
    keys.fill(false);
}

void Chip8Input::setKey(uint8_t key, bool pressed) {
    if (key < NUM_KEYS) keys[key] = pressed;
}

bool Chip8Input::isPressed(uint8_t key) const {
    if (key < NUM_KEYS) return keys[key];
    return false;
}

int Chip8Input::getPressedKey() const {
    for (uint8_t i = 0; i < NUM_KEYS; ++i) {
        if (keys[i]) return i;
    }
    return -1;
}

std::array<bool, Chip8Input::NUM_KEYS>& Chip8Input::getKeys() { return keys; }
const std::array<bool, Chip8Input::NUM_KEYS>& Chip8Input::getKeys() const { return keys; } 