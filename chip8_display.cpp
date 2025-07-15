#include "chip8_display.h"
#include <algorithm>
#include <iostream>

Chip8Display::Chip8Display() : mode(Mode::LowRes), colorMode(ColorMode::Mono), activePlanes(1) {
    clear();
}

void Chip8Display::clear() {
    for (auto& row : fb) row.fill(0);
}

bool Chip8Display::drawSprite(uint8_t x, uint8_t y, const uint8_t* spriteData, uint8_t numRows, uint8_t planeMask) {
    bool collision = false;
    int w = width();
    int h = height();
    for (uint8_t row = 0; row < numRows; ++row) {
        uint8_t spriteByte = spriteData[row];
        for (uint8_t col = 0; col < 8; ++col) {
            if ((spriteByte & (0x80 >> col)) != 0) {
                int px = (x + col) % w;
                int py = (y + row) % h;
                if (colorMode == ColorMode::XOCHIP_2BPP) {
                    for (int plane = 0; plane < XOCHIP_PLANES; ++plane) {
                        if (planeMask & (1 << plane)) {
                            uint8_t bit = (fb[py][px] >> plane) & 1;
                            if (bit) collision = true;
                            fb[py][px] ^= (1 << plane);
                        }
                    }
                } else {
                    if (fb[py][px]) collision = true;
                    fb[py][px] ^= 1;
                }
            }
        }
    }
    return collision;
}

uint8_t Chip8Display::getPixel(int x, int y) const {
    int w = width();
    int h = height();
    if (x < 0 || x >= w || y < 0 || y >= h) return 0;
    return fb[y][x];
}

void Chip8Display::setPixel(int x, int y, uint8_t value) {
    int w = width();
    int h = height();
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    fb[y][x] = value;
}

const std::array<std::array<uint8_t, Chip8Display::HIRES_WIDTH>, Chip8Display::HIRES_HEIGHT>& Chip8Display::framebuffer() const {
    return fb;
}

std::array<std::array<uint8_t, Chip8Display::HIRES_WIDTH>, Chip8Display::HIRES_HEIGHT>& Chip8Display::framebuffer() {
    return fb;
}

void Chip8Display::setMode(Mode m) {
    mode = m;
    clear();
}

Chip8Display::Mode Chip8Display::getMode() const {
    return mode;
}

int Chip8Display::width() const {
    return (mode == Mode::HighRes) ? HIRES_WIDTH : LOWRES_WIDTH;
}

int Chip8Display::height() const {
    return (mode == Mode::HighRes) ? HIRES_HEIGHT : LOWRES_HEIGHT;
}

void Chip8Display::setColorMode(ColorMode m) {
    colorMode = m;
    clear();
}

Chip8Display::ColorMode Chip8Display::getColorMode() const {
    return colorMode;
}

void Chip8Display::setActivePlanes(uint8_t mask) {
    activePlanes = mask;
}

uint8_t Chip8Display::getActivePlanes() const {
    return activePlanes;
}


bool Chip8Display::scrollUp(uint8_t lines) {
    int w = width();
    int h = height();
    if (lines == 0) return true;
    if (lines >= h) {
        clear();
        return true;
    }
    if (colorMode == ColorMode::XOCHIP_2BPP) {
        for (int plane = 0; plane < XOCHIP_PLANES; ++plane) {
            if (!(activePlanes & (1 << plane))) continue;
            for (int y = 0; y < h - lines; ++y) {
                for (int x = 0; x < w; ++x) {
                    uint8_t src = (fb[y + lines][x] >> plane) & 1;
                    fb[y][x] = (fb[y][x] & ~(1 << plane)) | (src << plane);
                }
            }
            for (int y = h - lines; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    fb[y][x] &= ~(1 << plane);
                }
            }
        }
    } else {
        for (int y = 0; y < h - lines; ++y) {
            for (int x = 0; x < w; ++x) {
                fb[y][x] = fb[y + lines][x] & 1;
            }
        }
        for (int y = h - lines; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                fb[y][x] = 0;
            }
        }
    }
    return true;
}

bool Chip8Display::scrollDown(uint8_t lines) {
    int w = width();
    int h = height();
    if (lines == 0) return true;
    if (lines >= h) {
        clear();
        return true;
    }
    if (colorMode == ColorMode::XOCHIP_2BPP) {
        for (int plane = 0; plane < XOCHIP_PLANES; ++plane) {
            if (!(activePlanes & (1 << plane))) continue;
            for (int y = h - 1; y >= lines; --y) {
                for (int x = 0; x < w; ++x) {
                    uint8_t src = (fb[y - lines][x] >> plane) & 1;
                    fb[y][x] = (fb[y][x] & ~(1 << plane)) | (src << plane);
                }
            }
            for (int y = 0; y < lines; ++y) {
                for (int x = 0; x < w; ++x) {
                    fb[y][x] &= ~(1 << plane);
                }
            }
        }
    } else {
        for (int y = h - 1; y >= lines; --y) {
            for (int x = 0; x < w; ++x) {
                fb[y][x] = fb[y - lines][x] & 1;
            }
        }
        for (int y = 0; y < lines; ++y) {
            for (int x = 0; x < w; ++x) {
                fb[y][x] = 0;
            }
        }
    }
    return true;
}

bool Chip8Display::scrollLeft() {
    int w = width();
    int h = height();
    int amount = 4;
    if (amount >= w) {
        clear();
        return true;
    }
    if (colorMode == ColorMode::XOCHIP_2BPP) {
        for (int plane = 0; plane < XOCHIP_PLANES; ++plane) {
            if (!(activePlanes & (1 << plane))) continue;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w - amount; ++x) {
                    uint8_t src = (fb[y][x + amount] >> plane) & 1;
                    fb[y][x] = (fb[y][x] & ~(1 << plane)) | (src << plane);
                }
                for (int x = w - amount; x < w; ++x) {
                    fb[y][x] &= ~(1 << plane);
                }
            }
        }
    } else {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w - amount; ++x) {
                fb[y][x] = fb[y][x + amount] & 1;
            }
            for (int x = w - amount; x < w; ++x) {
                fb[y][x] = 0;
            }
        }
    }
    return true;
}

bool Chip8Display::scrollRight() {
    int w = width();
    int h = height();
    int amount = 4; 
    if (amount >= w) {
        clear();
        return true;
    }
    if (colorMode == ColorMode::XOCHIP_2BPP) {
        for (int plane = 0; plane < XOCHIP_PLANES; ++plane) {
            if (!(activePlanes & (1 << plane))) continue;
            for (int y = 0; y < h; ++y) {
                for (int x = w - 1; x >= amount; --x) {
                    uint8_t src = (fb[y][x - amount] >> plane) & 1;
                    fb[y][x] = (fb[y][x] & ~(1 << plane)) | (src << plane);
                }
                for (int x = 0; x < amount; ++x) {
                    fb[y][x] &= ~(1 << plane);
                }
            }
        }
    } else {
        for (int y = 0; y < h; ++y) {
            for (int x = w - 1; x >= amount; --x) {
                fb[y][x] = fb[y][x - amount] & 1;
            }
            for (int x = 0; x < amount; ++x) {
                fb[y][x] = 0;
            }
        }
    }
    return true;
} 