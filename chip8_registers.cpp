// CHIP8CHAPA - CHIP-8 Registers implementation
// Handles V registers, I, PC, SP, and stack for CHIP-8

#include "chip8_registers.h"
#include <stdexcept>

Chip8Registers::Chip8Registers() : i(0), pc(0x200), sp(0) {
    v.fill(0);
    stack.fill(0);
}

uint8_t& Chip8Registers::V(size_t idx) {
    if (idx >= 16) throw std::out_of_range("V register index out of range");
    return v[idx];
}
const uint8_t& Chip8Registers::V(size_t idx) const {
    if (idx >= 16) throw std::out_of_range("V register index out of range");
    return v[idx];
}

uint16_t& Chip8Registers::I() { return i; }
const uint16_t& Chip8Registers::I() const { return i; }

uint16_t& Chip8Registers::PC() { return pc; }
const uint16_t& Chip8Registers::PC() const { return pc; }

uint8_t& Chip8Registers::SP() { return sp; }
const uint8_t& Chip8Registers::SP() const { return sp; }

void Chip8Registers::push(uint16_t value) {
    if (sp >= 16) throw std::overflow_error("Stack overflow");
    stack[sp++] = value;
}

uint16_t Chip8Registers::pop() {
    if (sp == 0) throw std::underflow_error("Stack underflow");
    return stack[--sp];
}

std::array<uint8_t, 16>& Chip8Registers::getV() { return v; }
const std::array<uint8_t, 16>& Chip8Registers::getV() const { return v; }
std::array<uint16_t, 16>& Chip8Registers::getStack() { return stack; }
const std::array<uint16_t, 16>& Chip8Registers::getStack() const { return stack; } 