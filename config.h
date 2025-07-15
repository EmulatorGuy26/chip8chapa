#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>

struct Config {
    std::vector<std::string> recentROMs;
    int audioVolume = 100;
    bool audioMuted = false;
    std::array<int32_t, 16> inputKeymap = {};
    int windowScale = 10;
    int mode = 0;

    void load(const std::string& path);
    void save(const std::string& path) const;
};

extern Config g_config; 