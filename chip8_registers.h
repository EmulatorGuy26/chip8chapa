// CHIP8CHAPA - CHIP-8 Registers header
// Declares V registers, I, PC, SP, and stack API for CHIP-8

#ifndef CHIP8_REGISTERS_H
#define CHIP8_REGISTERS_H

#include <array>
#include <cstdint>

class Chip8Registers {
public:
    Chip8Registers();

    // Access general purpose registers V0-VF
    uint8_t& V(size_t idx);
    const uint8_t& V(size_t idx) const;

    // Access index register
    uint16_t& I();
    const uint16_t& I() const;

    // Access program counter
    uint16_t& PC();
    const uint16_t& PC() const;

    // Access stack pointer
    uint8_t& SP();
    const uint8_t& SP() const;

    // Stack operations
    void push(uint16_t value);
    uint16_t pop();

    // Direct access to stack array
    std::array<uint16_t, 16>& getStack();
    const std::array<uint16_t, 16>& getStack() const;

    // Direct access to V registers array
    std::array<uint8_t, 16>& getV();
    const std::array<uint8_t, 16>& getV() const;

private:
    std::array<uint8_t, 16> v;
    uint16_t i;
    uint16_t pc;
    uint8_t sp;
    std::array<uint16_t, 16> stack;
};

#endif