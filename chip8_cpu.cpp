// CHIP8CHAPA - CHIP-8 CPU core implementation
// Handles instruction decoding, execution, and state serialization for CHIP-8, SCHIP, XO-CHIP

#include "chip8_cpu.h"
#include <stdexcept>
#include <iostream>
#include <random>
#include <array>
#include <chrono>
#include <fstream>
#include <string>

namespace {
    // Size of XO-CHIP audio pattern buffer (16 patterns, 16 bytes each)
    constexpr size_t XOCHIP_AUDIO_BUFFER_SIZE = 16 * 16;
}

class Chip8CPU_AudioBuffer {
public:
    std::array<uint8_t, XOCHIP_AUDIO_BUFFER_SIZE> buffer{};
};

Chip8CPU::Chip8CPU(Variant variant)
    : mode(variant),
      mem(variant == Variant::XOCHIP ? Chip8Memory::XOCHIP_MEMORY_SIZE :
          (variant == Variant::SCHIP ? Chip8Memory::SCHIP_MEMORY_SIZE : Chip8Memory::CHIP8_MEMORY_SIZE))
{}

Chip8Memory& Chip8CPU::memory() { return mem; }
Chip8Registers& Chip8CPU::registers() { return regs; }
Chip8Timers& Chip8CPU::timers() { return tmr; }
Chip8Input& Chip8CPU::input() { return inp; }
Chip8Display& Chip8CPU::display() { return disp; }
Chip8Sound& Chip8CPU::sound() { return snd; }
Chip8CPU::Variant Chip8CPU::getVariant() const { return mode; }

void Chip8CPU::setQuirks(const Quirks& q) { quirks = q; }
Chip8CPU::Quirks Chip8CPU::getQuirks() const { return quirks; }

uint16_t Chip8CPU::fetchOpcode() {
    uint16_t pc = regs.PC();
    uint8_t high = mem.read(pc);
    uint8_t low = mem.read(pc + 1);
    return (high << 8) | low;
}

void Chip8CPU::step() {
    uint16_t opcode = fetchOpcode();
    regs.PC() += 2;
    executeOpcode(opcode);
    if (tmr.getSound() > 0) {
        snd.start();
    } else {
        snd.stop();
    }
}

void Chip8CPU::executeOpcode(uint16_t opcode) {
    uint8_t n1 = (opcode & 0xF000) >> 12;
    uint8_t n2 = (opcode & 0x0F00) >> 8;
    uint8_t n3 = (opcode & 0x00F0) >> 4;
    uint8_t n4 = (opcode & 0x000F);
    uint16_t nnn = opcode & 0x0FFF;
    uint8_t nn = opcode & 0x00FF;
    uint8_t x = n2;
    uint8_t y = n3;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    if (mode == Variant::SCHIP || mode == Variant::XOCHIP) {
        if ((opcode & 0xFFF0) == 0x00C0) {
            uint8_t n = opcode & 0x000F;
            int lines = 0;
            if (mode == Variant::XOCHIP) {
                lines = (disp.getMode() == Chip8Display::Mode::LowRes) ? n : n * 2;
            } else {
                lines = n;
            }
            disp.scrollDown(lines);
            return;
        }
        if (opcode == 0x00FB) {
            disp.scrollRight();
            return;
        }
        if (opcode == 0x00FC) {
            disp.scrollLeft();
            return;
        }
        if ((opcode & 0xFFF0) == 0x00FA) {
            uint8_t n = opcode & 0x000F;
            int lines = 0;
            if (mode == Variant::XOCHIP) {
                lines = (disp.getMode() == Chip8Display::Mode::LowRes) ? n : n * 2;
            } else {
                lines = n;
            }
            disp.scrollUp(lines);
            return;
        }
        if (opcode == 0x00FE) {
            disp.setMode(Chip8Display::Mode::LowRes);
            return;
        }
        if (opcode == 0x00FF) {
            disp.setMode(Chip8Display::Mode::HighRes);
            return;
        }
        if (opcode == 0x00FD) {
            return;
        }
        if ((opcode & 0xF00F) == 0xD000 && n4 == 0) {
            uint8_t vx = regs.V(x);
            uint8_t vy = regs.V(y);
            bool collision = false;
            for (int row = 0; row < 16; ++row) {
                uint16_t spriteRow = (mem.read(regs.I() + row * 2) << 8) | mem.read(regs.I() + row * 2 + 1);
                for (int col = 0; col < 16; ++col) {
                    if ((spriteRow & (0x8000 >> col)) != 0) {
                        int px = (vx + col) % disp.width();
                        int py = (vy + row) % disp.height();
                        if (disp.getPixel(px, py)) collision = true;
                        disp.setPixel(px, py, disp.getPixel(px, py) ^ true);
                    }
                }
            }
            regs.V(0xF) = collision ? 1 : 0;
            return;
        }
    }

    switch (n1) {
    case 0x0:
        switch (opcode) {
        case 0x00E0:
            disp.clear();
            break;
        case 0x00EE:
            regs.PC() = regs.pop();
            break;
        default:
            break;
        }
        break;
    case 0x1:
        regs.PC() = nnn;
        break;
    case 0x2:
        regs.push(regs.PC());
        regs.PC() = nnn;
        break;
    case 0x3:
        if (regs.V(x) == nn) regs.PC() += 2;
        break;
    case 0x4:
        if (regs.V(x) != nn) regs.PC() += 2;
        break;
    case 0x5:
        if (n4 == 0 && regs.V(x) == regs.V(y)) regs.PC() += 2;
        else if (n4 == 2 && mode == Variant::XOCHIP) {
            uint8_t tmp = regs.V(x);
            regs.V(x) = regs.V(y);
            regs.V(y) = tmp;
        }
        else if (n4 == 3 && mode == Variant::XOCHIP) {
            regs.V(y) = regs.V(x);
            regs.V(x) = 0;
        }
        break;
    case 0x6:
        regs.V(x) = nn;
        break;
    case 0x7:
        regs.V(x) = (regs.V(x) + nn) & 0xFF;
        break;
    case 0x8:
        switch (n4) {
        case 0x0:
            regs.V(x) = regs.V(y);
            break;
        case 0x1:
            regs.V(x) |= regs.V(y);
            if (mode == Variant::CHIP8) regs.V(0xF) = 0;
            break;
        case 0x2:
            regs.V(x) &= regs.V(y);
            if (mode == Variant::CHIP8) regs.V(0xF) = 0;
            break;
        case 0x3:
            regs.V(x) ^= regs.V(y);
            if (mode == Variant::CHIP8) regs.V(0xF) = 0;
            break;
        case 0x4: {
            uint8_t vx = regs.V(x);
            uint8_t vy = regs.V(y);
            uint16_t sum = vx + vy;
            uint8_t result = sum & 0xFF;
            uint8_t vf = (sum > 0xFF) ? 1 : 0;
            regs.V(x) = result;
            regs.V(0xF) = vf;
            break;
        }
        case 0x5: {
            uint8_t vx = regs.V(x);
            uint8_t vy = regs.V(y);
            uint8_t result = (vx - vy) & 0xFF;
            uint8_t vf = (vx >= vy) ? 1 : 0;
            regs.V(x) = result;
            regs.V(0xF) = vf;
            break;
        }
        case 0x6: {
            uint8_t vx = regs.V(x);
            uint8_t vy = regs.V(y);
            uint8_t src = quirks.shiftUsesVy ? vy : vx;
            uint8_t result = src >> 1;
            uint8_t vf = src & 0x1;
            regs.V(x) = result;
            regs.V(0xF) = vf;
            break;
        }
        case 0x7: {
            uint8_t vx = regs.V(x);
            uint8_t vy = regs.V(y);
            uint8_t result = (vy - vx) & 0xFF;
            uint8_t vf = (vy >= vx) ? 1 : 0;
            regs.V(x) = result;
            regs.V(0xF) = vf;
            break;
        }
        case 0xE: {
            uint8_t vx = regs.V(x);
            uint8_t vy = regs.V(y);
            uint8_t src = quirks.shiftUsesVy ? vy : vx;
            uint8_t result = (src << 1) & 0xFF;
            uint8_t vf = (src & 0x80) >> 7;
            regs.V(x) = result;
            regs.V(0xF) = vf;
            break;
        }
        }
        break;
    case 0x9:
        if (n4 == 0 && regs.V(x) != regs.V(y)) regs.PC() += 2;
        break;
    case 0xA:
        regs.I() = nnn;
        break;
    case 0xB:
        regs.PC() = nnn + (quirks.jumpWithVx ? regs.V(x) : regs.V(0));
        break;
    case 0xC:
        regs.V(x) = dis(gen) & nn;
        break;
    case 0xD: {
        uint8_t vx = regs.V(x);
        uint8_t vy = regs.V(y);
        uint8_t n = n4;
        bool collision = false;
        static auto lastDraw = std::chrono::high_resolution_clock::now();
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastDraw).count();
        if (elapsed < (1.0 / 60.0)) {
            regs.PC() -= 2;
            break;
        }
        lastDraw = now;
        if (mode == Variant::CHIP8) {
            int w = disp.width();
            int h = disp.height();
            int startX = vx % w;
            int startY = vy % h;
            for (uint8_t row = 0; row < n; ++row) {
                uint8_t spriteByte = mem.read(regs.I() + row);
                int py = startY + row;
                if (py >= h) continue;
                for (uint8_t col = 0; col < 8; ++col) {
                    int px = startX + col;
                    if (px >= w) continue;
                    if ((spriteByte & (0x80 >> col)) != 0) {
                        if (disp.getPixel(px, py)) collision = true;
                        disp.setPixel(px, py, disp.getPixel(px, py) ^ 1);
                    }
                }
            }
        } else {
            if (mode == Variant::SCHIP) {
                int w = disp.width();
                int h = disp.height();
                int startX = vx & (w - 1);
                int startY = vy & (h - 1);
                for (uint8_t row = 0; row < n; ++row) {
                    uint8_t spriteByte = mem.read(regs.I() + row);
                    int py = startY + row;
                    if (py >= h) continue;
                    for (uint8_t col = 0; col < 8; ++col) {
                        int px = startX + col;
                        if (px >= w) continue;
                        if ((spriteByte & (0x80 >> col)) != 0) {
                            if (disp.getPixel(px, py)) collision = true;
                            disp.setPixel(px, py, disp.getPixel(px, py) ^ 1);
                        }
                    }
                }
            } else {
                collision = disp.drawSprite(vx, vy, mem.data() + regs.I(), n);
            }
        }
        regs.V(0xF) = collision ? 1 : 0;
        break;
    }
    case 0xE:
        switch (nn) {
        case 0x9E:
            if (inp.isPressed(regs.V(x))) regs.PC() += 2;
            break;
        case 0xA1:
            if (!inp.isPressed(regs.V(x))) regs.PC() += 2;
            break;
        }
        break;
    case 0xF:
        switch (nn) {
        case 0x07:
            regs.V(x) = tmr.getDelay();
            break;
        case 0x0A: {
            static bool waitingForRelease = false;
            static int lastKey = -1;
            int key = inp.getPressedKey();
            if (!waitingForRelease) {
                if (key == -1) {
                    regs.PC() -= 2;
                } else {
                    regs.V(x) = key;
                    waitingForRelease = true;
                    lastKey = key;
                    regs.PC() -= 2;
                }
            } else {
                if (key == lastKey) {
                    regs.PC() -= 2;
                } else {
                    waitingForRelease = false;
                    lastKey = -1;
                }
            }
            break;
        }
        case 0x15:
            tmr.setDelay(regs.V(x));
            break;
        case 0x18:
            tmr.setSound(regs.V(x));
            break;
        case 0x1E:
            regs.I() = (regs.I() + regs.V(x)) & 0xFFFF;
            break;
        case 0x29:
            regs.I() = Chip8Memory::FONTSET_START + (regs.V(x) & 0xF) * 5;
            break;
        case 0x33: {
            uint8_t value = regs.V(x);
            mem.write(regs.I(), value / 100);
            mem.write(regs.I() + 1, (value / 10) % 10);
            mem.write(regs.I() + 2, value % 10);
            break;
        }
        case 0x55:
            for (uint8_t i = 0; i <= x; ++i) mem.write(regs.I() + i, regs.V(i));
            if (quirks.loadStoreIncrementI) regs.I() += x + 1;
            break;
        case 0x65:
            for (uint8_t i = 0; i <= x; ++i) regs.V(i) = mem.read(regs.I() + i);
            if (quirks.loadStoreIncrementI) regs.I() += x + 1;
            break;
        case 0x01:
            if (mode == Variant::XOCHIP) {
                disp.setActivePlanes(regs.V(x));
            }
            break;
        if ((opcode & 0xFFF0) == 0x00D0 && mode == Variant::XOCHIP) {
            uint8_t n = opcode & 0x000F;
            if (n < 16) {
                snd.playPattern(&audioBuffer[n * 16]);
            }
            return;
        }
        case 0x75:
            if (mode == Variant::XOCHIP) {
                for (uint8_t i = 0; i <= x; ++i) {
                    audioBuffer[i] = regs.V(i);
                }
            }
            break;
        case 0x85:
            if (mode == Variant::XOCHIP) {
                for (uint8_t i = 0; i <= x; ++i) {
                    regs.V(i) = audioBuffer[i];
                }
            }
            break;
        }
        break;
    default:
        break;
    }
}

bool Chip8CPU::saveState(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    int modeInt = static_cast<int>(mode);
    out.write(reinterpret_cast<const char*>(&modeInt), sizeof(modeInt));
    out.write(reinterpret_cast<const char*>(&quirks), sizeof(quirks));
    size_t memSize = mem.size();
    out.write(reinterpret_cast<const char*>(&memSize), sizeof(memSize));
    out.write(reinterpret_cast<const char*>(mem.data()), memSize);
    out.write(reinterpret_cast<const char*>(regs.getV().data()), regs.getV().size());
    out.write(reinterpret_cast<const char*>(&regs.I()), sizeof(uint16_t));
    out.write(reinterpret_cast<const char*>(&regs.PC()), sizeof(uint16_t));
    out.write(reinterpret_cast<const char*>(&regs.SP()), sizeof(uint8_t));
    out.write(reinterpret_cast<const char*>(regs.getStack().data()), regs.getStack().size() * sizeof(uint16_t));
    uint8_t delay = tmr.getDelay();
    uint8_t sound = tmr.getSound();
    out.write(reinterpret_cast<const char*>(&delay), sizeof(delay));
    out.write(reinterpret_cast<const char*>(&sound), sizeof(sound));
    out.write(reinterpret_cast<const char*>(inp.getKeys().data()), inp.getKeys().size());
    for (const auto& row : disp.fb) {
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
    int modeVal = static_cast<int>(disp.getMode());
    int colorModeVal = static_cast<int>(disp.getColorMode());
    uint8_t activePlanes = disp.getActivePlanes();
    out.write(reinterpret_cast<const char*>(&modeVal), sizeof(modeVal));
    out.write(reinterpret_cast<const char*>(&colorModeVal), sizeof(colorModeVal));
    out.write(reinterpret_cast<const char*>(&activePlanes), sizeof(activePlanes));
    int phase = snd.getPhase();
    out.write(reinterpret_cast<const char*>(&phase), sizeof(phase));
    bool muted = snd.getMuted();
    out.write(reinterpret_cast<const char*>(&muted), sizeof(muted));
    int volume = snd.getVolume();
    out.write(reinterpret_cast<const char*>(&volume), sizeof(volume));
    bool buzzer = snd.isOn();
    out.write(reinterpret_cast<const char*>(&buzzer), sizeof(buzzer));
    bool playing = snd.getPlaying();
    out.write(reinterpret_cast<const char*>(&playing), sizeof(playing));
    out.write(reinterpret_cast<const char*>(audioBuffer.data()), audioBuffer.size());
    return !!out;
}

bool Chip8CPU::loadState(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    int modeInt = 0;
    in.read(reinterpret_cast<char*>(&modeInt), sizeof(modeInt));
    mode = static_cast<Variant>(modeInt);
    in.read(reinterpret_cast<char*>(&quirks), sizeof(quirks));
    size_t memSize = 0;
    in.read(reinterpret_cast<char*>(&memSize), sizeof(memSize));
    if (memSize != mem.size()) return false;
    in.read(reinterpret_cast<char*>(mem.data()), memSize);
    in.read(reinterpret_cast<char*>(regs.getV().data()), regs.getV().size());
    in.read(reinterpret_cast<char*>(&regs.I()), sizeof(uint16_t));
    in.read(reinterpret_cast<char*>(&regs.PC()), sizeof(uint16_t));
    in.read(reinterpret_cast<char*>(&regs.SP()), sizeof(uint8_t));
    in.read(reinterpret_cast<char*>(regs.getStack().data()), regs.getStack().size() * sizeof(uint16_t));
    uint8_t delay = 0, sound = 0;
    in.read(reinterpret_cast<char*>(&delay), sizeof(delay));
    in.read(reinterpret_cast<char*>(&sound), sizeof(sound));
    tmr.setDelay(delay);
    tmr.setSound(sound);
    in.read(reinterpret_cast<char*>(inp.getKeys().data()), inp.getKeys().size());
    for (auto& row : disp.fb) {
        in.read(reinterpret_cast<char*>(row.data()), row.size());
    }
    int modeVal = 0, colorModeVal = 0;
    uint8_t activePlanes = 0;
    in.read(reinterpret_cast<char*>(&modeVal), sizeof(modeVal));
    in.read(reinterpret_cast<char*>(&colorModeVal), sizeof(colorModeVal));
    in.read(reinterpret_cast<char*>(&activePlanes), sizeof(activePlanes));
    disp.mode = static_cast<Chip8Display::Mode>(modeVal);
    disp.colorMode = static_cast<Chip8Display::ColorMode>(colorModeVal);
    disp.activePlanes = activePlanes;
    int phase = 0;
    in.read(reinterpret_cast<char*>(&phase), sizeof(phase));
    snd.setPhase(phase);
    bool muted = false;
    in.read(reinterpret_cast<char*>(&muted), sizeof(muted));
    snd.setMuted(muted);
    int volume = 100;
    in.read(reinterpret_cast<char*>(&volume), sizeof(volume));
    snd.setVolume(volume);
    bool buzzer = false;
    in.read(reinterpret_cast<char*>(&buzzer), sizeof(buzzer));
    if (buzzer) snd.start(); else snd.stop();
    bool playing = false;
    in.read(reinterpret_cast<char*>(&playing), sizeof(playing));
    snd.setPlaying(playing);
    in.read(reinterpret_cast<char*>(audioBuffer.data()), audioBuffer.size());
    return !!in;
} 