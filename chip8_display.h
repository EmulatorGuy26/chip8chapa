// CHIP8CHAPA - CHIP-8 Display header
// Declares the display/framebuffer, drawing, scrolling, and color mode APIs

#pragma once
#include <array>
#include <cstdint>

class Chip8Display {
public:
    static constexpr int LOWRES_WIDTH = 64;
    static constexpr int LOWRES_HEIGHT = 32;
    static constexpr int HIRES_WIDTH = 128;
    static constexpr int HIRES_HEIGHT = 64;
    static constexpr int XOCHIP_PLANES = 2; // 2bpp (2 planes)

    // Display mode: low-res (64x32) or high-res (128x64)
    enum class Mode {
        LowRes,
        HighRes
    };
    // Color mode: monochrome or XO-CHIP 2bpp
    enum class ColorMode {
        Mono,
        XOCHIP_2BPP
    };

    Chip8Display();

    // Clears the display (all pixels off)
    void clear();
    // Draws a sprite at (x, y) with N bytes from spriteData, returns true if any pixel was unset (collision)
    bool drawSprite(uint8_t x, uint8_t y, const uint8_t* spriteData, uint8_t numRows, uint8_t planeMask = 1);
    // Gets the pixel state (for mono: 0/1, for color: 0-3)
    uint8_t getPixel(int x, int y) const;
    // Sets a pixel state (for testing/debugging)
    void setPixel(int x, int y, uint8_t value);
    // Access the framebuffer (for UI)
    std::array<std::array<uint8_t, HIRES_WIDTH>, HIRES_HEIGHT>& framebuffer();
    const std::array<std::array<uint8_t, HIRES_WIDTH>, HIRES_HEIGHT>& framebuffer() const;

    // Set/get display mode (low-res or high-res)
    void setMode(Mode mode);
    Mode getMode() const;
    int width() const;
    int height() const;

    // Set/get color mode (mono or XO-CHIP 2bpp)
    void setColorMode(ColorMode mode);
    ColorMode getColorMode() const;
    void setActivePlanes(uint8_t mask); // For XO-CHIP plane selection
    uint8_t getActivePlanes() const;

    // Scroll up by N lines (0 = no scroll, clears if >= height)
    bool scrollUp(uint8_t lines);
    // Scroll down by N lines (0 = no scroll, clears if >= height)
    bool scrollDown(uint8_t lines);
    // Scroll left/right by 4 pixels (SCHIP/XO-CHIP)
    bool scrollLeft();
    bool scrollRight();

public:
    std::array<std::array<uint8_t, HIRES_WIDTH>, HIRES_HEIGHT> fb; // 0-3 for 2bpp, 0/1 for mono
    friend class Chip8CPU;
private:
    Mode mode;
    ColorMode colorMode;
    uint8_t activePlanes; // Bitmask for XO-CHIP plane selection
};