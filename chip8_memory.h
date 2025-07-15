// CHIP8CHAPA - CHIP-8 Memory header
// Declares memory array, ROM loading, and fontset API

#ifndef CHIP8_MEMORY_H
#define CHIP8_MEMORY_H

#include <vector>
#include <cstdint>

class Chip8Memory {
public:
    static constexpr size_t CHIP8_MEMORY_SIZE = 4096;
    static constexpr size_t SCHIP_MEMORY_SIZE = 8192;
    static constexpr size_t XOCHIP_MEMORY_SIZE = 65536;
    static constexpr uint16_t PROGRAM_START = 0x200;
    static constexpr size_t FONTSET_START = 0x50;
    static constexpr size_t FONTSET_SIZE = 80; // 16 characters * 5 bytes each

    explicit Chip8Memory(size_t size = CHIP8_MEMORY_SIZE);

    // Read/write memory at address
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);
    // Load a ROM into memory (starting at PROGRAM_START)
    void loadROM(const std::vector<uint8_t>& rom);
    // Load the CHIP-8 fontset into memory
    void loadFontset();

    size_t size() const;

    // Direct access to memory array
    uint8_t* data();
    const uint8_t* data() const;

private:
    std::vector<uint8_t> memory;
};

#endif