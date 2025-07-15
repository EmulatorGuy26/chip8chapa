// CHIP8CHAPA - CHIP-8 Input header
// Declares keypad state and input API for CHIP-8 (0-F)

#pragma once
#include <array>
#include <cstdint>

class Chip8Input {
public:
    static constexpr size_t NUM_KEYS = 16;

    Chip8Input();

    // Set key state (pressed or released)
    void setKey(uint8_t key, bool pressed);
    // Get key state
    bool isPressed(uint8_t key) const;
    // Get currently pressed key (for FX0A), returns -1 if none
    int getPressedKey() const;

    // Direct access to key state array
    std::array<bool, NUM_KEYS>& getKeys();
    const std::array<bool, NUM_KEYS>& getKeys() const;

private:
    std::array<bool, NUM_KEYS> keys;
};