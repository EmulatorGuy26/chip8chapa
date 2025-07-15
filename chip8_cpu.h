// CHIP8CHAPA - CHIP-8 CPU core header
// Declares the CPU, quirks, and all major subsystems (memory, registers, timers, input, display, sound)

#pragma once
#include "chip8_memory.h"
#include "chip8_registers.h"
#include "chip8_timers.h"
#include "chip8_input.h"
#include "chip8_display.h"
#include "chip8_sound.h"
#include <cstdint>
#include <array>
#include <string>

// Main CHIP-8 CPU class: emulates all instructions and manages state
class Chip8CPU {
public:
    enum class Variant {
        CHIP8,   // Original CHIP-8
        SCHIP,   // SuperChip
        XOCHIP   // XO-Chip extension
    };
    Chip8CPU(const Chip8CPU&) = delete;
    Chip8CPU& operator=(const Chip8CPU&) = delete;
    Chip8CPU(Chip8CPU&&) = delete;
    Chip8CPU& operator=(Chip8CPU&&) = delete;

    // Emulation quirks for compatibility with different interpreters
    struct Quirks {
        bool shiftUsesVy = false;       // 8XY6/8XYE: use Vy as source (true) or Vx (false)
        bool loadStoreIncrementI = true;  // FX55/FX65: increment I after operation (true = modern, false = original)
        bool jumpWithVx = false;        // BNNN: use VX instead of V0
    };

    explicit Chip8CPU(Variant variant = Variant::CHIP8);

    void setQuirks(const Quirks& quirks);
    Quirks getQuirks() const;

    // Executes one instruction (fetch, decode, execute)
    void step();

    // Access to subsystems for testing/debugging
    Chip8Memory& memory();
    Chip8Registers& registers();
    Chip8Timers& timers();
    Chip8Input& input();
    Chip8Display& display();
    Chip8Sound& sound();

    Variant getVariant() const;

    // Save/load full emulator state to a file (for save states)
    bool saveState(const std::string& path) const;
    bool loadState(const std::string& path);

private:
    Variant mode;
    Quirks quirks;
    Chip8Memory mem;
    Chip8Registers regs;
    Chip8Timers tmr;
    Chip8Input inp;
    Chip8Display disp;
    Chip8Sound snd;
    std::array<uint8_t, 16 * 16> audioBuffer{}; // XO-CHIP audio pattern buffer

    // Fetches the next opcode (2 bytes) from memory at PC
    uint16_t fetchOpcode();
    // Decodes and executes the given opcode
    void executeOpcode(uint16_t opcode);
};
