// CHIP8CHAPA - CHIP-8 Memory implementation
// Handles memory array, ROM loading, and fontset for CHIP-8/SCHIP/XO-CHIP

#include "chip8_memory.h"
#include <stdexcept>
#include <algorithm>

static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8Memory::Chip8Memory(size_t size) : memory(size, 0) {
    loadFontset();
}

uint8_t Chip8Memory::read(uint16_t address) const {
    if (address >= memory.size()) {
        throw std::out_of_range("Memory read out of range");
    }
    return memory[address];
}

void Chip8Memory::write(uint16_t address, uint8_t value) {
    if (address >= memory.size()) {
        throw std::out_of_range("Memory write out of range");
    }
    memory[address] = value;
}

void Chip8Memory::loadROM(const std::vector<uint8_t>& rom) {
    if (rom.size() + PROGRAM_START > memory.size()) {
        throw std::runtime_error("ROM too large to fit in memory");
    }
    std::copy(rom.begin(), rom.end(), memory.begin() + PROGRAM_START);
}

void Chip8Memory::loadFontset() {
    if (FONTSET_START + FONTSET_SIZE <= memory.size()) {
        std::copy(chip8_fontset, chip8_fontset + FONTSET_SIZE, memory.begin() + FONTSET_START);
    }
}

size_t Chip8Memory::size() const {
    return memory.size();
}

uint8_t* Chip8Memory::data() {
    return memory.data();
}

const uint8_t* Chip8Memory::data() const {
    return memory.data();
} 