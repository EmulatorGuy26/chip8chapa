#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>

Config g_config;

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

void Config::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (key == "recentROMs") {
            recentROMs.clear();
            std::istringstream ss(value);
            std::string rom;
            while (std::getline(ss, rom, '|')) {
                if (!rom.empty()) recentROMs.push_back(rom);
            }
        } else if (key == "audioVolume") {
            audioVolume = std::stoi(value);
        } else if (key == "audioMuted") {
            audioMuted = (value == "1" || value == "true");
        } else if (key == "inputKeymap") {
            std::istringstream ss(value);
            for (int i = 0; i < 16; ++i) {
                std::string k;
                if (!std::getline(ss, k, ',')) break;
                inputKeymap[i] = std::stoi(k);
            }
        } else if (key == "windowScale") {
            windowScale = std::stoi(value);
        } else if (key == "mode") {
            mode = std::stoi(value);
        }
    }
}

void Config::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) return;
    out << "recentROMs=";
    for (size_t i = 0; i < recentROMs.size(); ++i) {
        if (i > 0) out << "|";
        out << recentROMs[i];
    }
    out << "\n";
    out << "audioVolume=" << audioVolume << "\n";
    out << "audioMuted=" << (audioMuted ? 1 : 0) << "\n";
    out << "inputKeymap=";
    for (int i = 0; i < 16; ++i) {
        if (i > 0) out << ",";
        out << inputKeymap[i];
    }
    out << "\n";
    out << "windowScale=" << windowScale << "\n";
    out << "mode=" << mode << "\n";
} 