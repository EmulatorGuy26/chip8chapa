// CHIP8CHAPA - Main entry point and Windows UI integration for the CHIP-8 emulator
// Handles SDL2 setup, Win32 menu/dialogs, persistent config, and main emulation loop

#include <SDL.h>
#include "chip8_cpu.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <vector>
#include <functional>
#include <deque>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <SDL_syswm.h>
#include <commctrl.h>
#include <direct.h>
#pragma comment(lib, "comctl32.lib")
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "config.h"
#include <map>

// Mapping table: Windows VK codes to SDL_Keycode values for remapping
static std::map<UINT, SDL_Keycode> vkToSDLKey = {
    { 'A', SDLK_a }, { 'B', SDLK_b }, { 'C', SDLK_c }, { 'D', SDLK_d }, { 'E', SDLK_e },
    { 'F', SDLK_f }, { 'G', SDLK_g }, { 'H', SDLK_h }, { 'I', SDLK_i }, { 'J', SDLK_j },
    { 'K', SDLK_k }, { 'L', SDLK_l }, { 'M', SDLK_m }, { 'N', SDLK_n }, { 'O', SDLK_o },
    { 'P', SDLK_p }, { 'Q', SDLK_q }, { 'R', SDLK_r }, { 'S', SDLK_s }, { 'T', SDLK_t },
    { 'U', SDLK_u }, { 'V', SDLK_v }, { 'W', SDLK_w }, { 'X', SDLK_x }, { 'Y', SDLK_y }, { 'Z', SDLK_z },
    { '0', SDLK_0 }, { '1', SDLK_1 }, { '2', SDLK_2 }, { '3', SDLK_3 }, { '4', SDLK_4 },
    { '5', SDLK_5 }, { '6', SDLK_6 }, { '7', SDLK_7 }, { '8', SDLK_8 }, { '9', SDLK_9 },
    { VK_NUMPAD0, SDLK_KP_0 }, { VK_NUMPAD1, SDLK_KP_1 }, { VK_NUMPAD2, SDLK_KP_2 }, { VK_NUMPAD3, SDLK_KP_3 },
    { VK_NUMPAD4, SDLK_KP_4 }, { VK_NUMPAD5, SDLK_KP_5 }, { VK_NUMPAD6, SDLK_KP_6 }, { VK_NUMPAD7, SDLK_KP_7 },
    { VK_NUMPAD8, SDLK_KP_8 }, { VK_NUMPAD9, SDLK_KP_9 },
    { VK_LEFT, SDLK_LEFT }, { VK_RIGHT, SDLK_RIGHT }, { VK_UP, SDLK_UP }, { VK_DOWN, SDLK_DOWN },
    { VK_SPACE, SDLK_SPACE }, { VK_RETURN, SDLK_RETURN }, { VK_BACK, SDLK_BACKSPACE },
    { VK_TAB, SDLK_TAB }, { VK_ESCAPE, SDLK_ESCAPE },
    { VK_F1, SDLK_F1 }, { VK_F2, SDLK_F2 }, { VK_F3, SDLK_F3 }, { VK_F4, SDLK_F4 }, { VK_F5, SDLK_F5 },
    { VK_F6, SDLK_F6 }, { VK_F7, SDLK_F7 }, { VK_F8, SDLK_F8 }, { VK_F9, SDLK_F9 }, { VK_F10, SDLK_F10 },
    { VK_F11, SDLK_F11 }, { VK_F12, SDLK_F12 },
    { VK_OEM_MINUS, SDLK_MINUS }, { VK_OEM_PLUS, SDLK_EQUALS }, { VK_OEM_4, SDLK_LEFTBRACKET },
    { VK_OEM_6, SDLK_RIGHTBRACKET }, { VK_OEM_1, SDLK_SEMICOLON }, { VK_OEM_7, SDLK_QUOTE },
    { VK_OEM_COMMA, SDLK_COMMA }, { VK_OEM_PERIOD, SDLK_PERIOD }, { VK_OEM_2, SDLK_SLASH },
    { VK_OEM_5, SDLK_BACKSLASH }, { VK_OEM_3, SDLK_BACKQUOTE }
};

// Returns the path to config.ini next to the executable (cross-platform)
std::string getConfigPath() {
    char exePath[1024] = {0};
#ifdef _WIN32
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    char* lastSlash = strrchr(exePath, '\\');
#else
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath)-1);
    if (len != -1) exePath[len] = '\0';
    char* lastSlash = strrchr(exePath, '/');
#endif
    if (lastSlash) *lastSlash = '\0';
    std::string configPath = std::string(exePath)
#ifdef _WIN32
        + "\\config.ini";
#else
        + "/config.ini";
#endif
    return configPath;
}

constexpr int LOWRES_SCALE = 10;
constexpr int HIRES_SCALE = 5;
constexpr int INSTR_PER_SECOND = 700;
constexpr int TIMER_HZ = 60;

// CHIP-8 keypad layout (default SDL key mapping)
SDL_Keycode keymap[16] = {
    SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
    SDLK_8, SDLK_9, SDLK_a, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f
};

// Resets keymap to default CHIP-8 layout
void restoreDefaultKeymap() {
    SDL_Keycode defaults[16] = {
        SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
        SDLK_8, SDLK_9, SDLK_a, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f
    };
    for (int i = 0; i < 16; ++i) keymap[i] = defaults[i];
}

// Holds up to 5 most recent ROM file paths
constexpr int MAX_RECENT_ROMS = 5;
std::deque<std::string> recentROMs;

// Adds a ROM path to the recent list, keeping it unique and capped
void addRecentROM(const std::string& path) {
    auto it = std::find(recentROMs.begin(), recentROMs.end(), path);
    if (it != recentROMs.end()) recentROMs.erase(it);
    recentROMs.push_front(path);
    if (recentROMs.size() > MAX_RECENT_ROMS) recentROMs.pop_back();
}

#ifdef _WIN32
std::string openFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "CHIP-8 ROMs (*.ch8;*.rom)\0*.ch8;*.rom\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) {
        return filename;
    }
    return "";
}
#else
#include <iostream>
std::string openFileDialog() {
    std::string path;
    std::getline(std::cin, path);
    return path;
}
#endif

void resizeWindow(SDL_Window* window, const Chip8Display& display) {
    int scale = (display.getMode() == Chip8Display::Mode::HighRes) ? HIRES_SCALE : LOWRES_SCALE;
    int w = display.width() * scale;
    int h = display.height() * scale;
    SDL_SetWindowSize(window, w, h);
}

void renderDisplay(SDL_Window* window, SDL_Renderer* renderer, const Chip8Display& display) {
    int winW, winH;
    SDL_GetWindowSize(window, &winW, &winH);
    int w = display.width();
    int h = display.height();
    float scaleX = static_cast<float>(winW) / w;
    float scaleY = static_cast<float>(winH) / h;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    if (scale < 1.0f) scale = 1.0f;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t pixel = display.getPixel(x, y);
            if (pixel) {
                uint8_t color = (display.getColorMode() == Chip8Display::ColorMode::XOCHIP_2BPP) ? (pixel * 85) : 255;
                SDL_SetRenderDrawColor(renderer, color, color, color, 255);
                SDL_Rect rect = { static_cast<int>(x * scale), static_cast<int>(y * scale), static_cast<int>(scale), static_cast<int>(scale) };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

#ifdef _WIN32
static int windowScale = LOWRES_SCALE;
static bool menuPaused = false;
static bool* g_romLoaded = nullptr;
static std::string* g_currentRomPath = nullptr;
static std::vector<uint8_t>* g_currentRomData = nullptr;
static bool* g_paused = nullptr;
static Chip8CPU* g_cpu = nullptr;
static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static Chip8Display::Mode* g_lastMode = nullptr;
static std::function<bool(const std::string&)>* g_loadROM = nullptr;
static HWND g_hwnd = nullptr;

static bool audioMuted = false;
static int audioVolume = 100;

static HWND g_audioDlg = nullptr;
#define AUDIO_SLIDER_ID 30001
#define AUDIO_MUTE_ID   30002
#define AUDIO_TEST_ID   30003

static HWND g_inputDlg = nullptr;
#define INPUT_GRID_BASE_ID 40000
#define INPUT_RESTORE_ID   40050
#define INPUT_CLOSE_ID     40051
static int selectedKey = -1;

void updateMenuBar();

LRESULT CALLBACK AudioDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hbrBkgnd = nullptr;
    static HWND hSlider = nullptr;
    if (!hbrBkgnd) hbrBkgnd = (HBRUSH)(COLOR_WINDOW+1);
    switch (msg) {
    case WM_CLOSE:
        g_audioDlg = nullptr;
        DestroyWindow(hwnd);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == AUDIO_MUTE_ID && g_cpu) {
            audioMuted = (IsDlgButtonChecked(hwnd, AUDIO_MUTE_ID) == BST_CHECKED);
            g_cpu->sound().setMuted(audioMuted);
            updateMenuBar();
        } else if (LOWORD(wParam) == AUDIO_TEST_ID && g_cpu) {
            g_cpu->sound().playTestBeep();
        }
        break;
    case WM_HSCROLL:
        if ((HWND)lParam && GetDlgCtrlID((HWND)lParam) == AUDIO_SLIDER_ID && g_cpu) {
            int pos = SendMessageA((HWND)lParam, TBM_GETPOS, 0, 0);
            audioVolume = pos;
            g_cpu->sound().setVolume(audioVolume);
            updateMenuBar();
        }
        break;
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_WIN95_CLASSES};
        InitCommonControlsEx(&icc);
        hSlider = CreateWindowExA(0, TRACKBAR_CLASS, "",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS,
            30, 30, 240, 30, hwnd, (HMENU)AUDIO_SLIDER_ID, GetModuleHandleA(NULL), NULL);
        SendMessageA(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessageA(hSlider, TBM_SETPOS, TRUE, audioVolume);
        CreateWindowExA(0, "STATIC", "Volume:", WS_CHILD | WS_VISIBLE,
            30, 10, 60, 20, hwnd, NULL, GetModuleHandleA(NULL), NULL);
        HWND hMute = CreateWindowExA(0, "BUTTON", "Mute",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            30, 70, 80, 24, hwnd, (HMENU)AUDIO_MUTE_ID, GetModuleHandleA(NULL), NULL);
        CreateWindowExA(0, "BUTTON", "Test Sound",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            190, 70, 80, 24, hwnd, (HMENU)AUDIO_TEST_ID, GetModuleHandleA(NULL), NULL);
        CheckDlgButton(hwnd, AUDIO_MUTE_ID, audioMuted ? BST_CHECKED : BST_UNCHECKED);
        ShowWindow(hwnd, SW_SHOW);
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(0,0,0));
        return (LRESULT)hbrBkgnd;
    }
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSCROLLBAR:
        return (LRESULT)hbrBkgnd;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ShowAudioDialog(HWND parent) {
    if (g_audioDlg) {
        SetForegroundWindow(g_audioDlg);
        return;
    }
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = AudioDlgProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "AudioSettingsWindow";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    g_audioDlg = CreateWindowExA(0, "AudioSettingsWindow", "Audio Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 180, parent, NULL, wc.hInstance, NULL);
}

std::string getKeyName(SDL_Keycode key) {
    const char* name = SDL_GetKeyName(key);
    return name ? name : "?";
}

INT_PTR CALLBACK InputRemapDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        selectedKey = -1;
        for (int i = 0; i < 16; ++i) {
            int row = i / 4, col = i % 4;
            char label[8];
            sprintf_s(label, sizeof(label), "%X", i);
            CreateWindowA("STATIC", label, WS_CHILD | WS_VISIBLE | SS_CENTER,
                30 + col * 150, 20 + row * 40, 30, 24, hWnd, (HMENU)(INPUT_GRID_BASE_ID + i), NULL, NULL);
        }
        for (int i = 0; i < 16; ++i) {
            int row = i / 4, col = i % 4;
            std::string btnText = getKeyName(keymap[i]);
            CreateWindowA("BUTTON", btnText.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                70 + col * 150, 20 + row * 40, 90, 24, hWnd, (HMENU)(INPUT_GRID_BASE_ID + 16 + i), NULL, NULL);
        }
        int dialogWidth = 650;
        int btnWidth = 130, closeWidth = 80, spacing = 30;
        int totalWidth = btnWidth + spacing + closeWidth;
        int startX = (dialogWidth - totalWidth) / 2;
        CreateWindowA("BUTTON", "Restore Defaults", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            startX, 220, btnWidth, 28, hWnd, (HMENU)INPUT_RESTORE_ID, NULL, NULL);
        CreateWindowA("BUTTON", "Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            startX + btnWidth + spacing, 220, closeWidth, 28, hWnd, (HMENU)INPUT_CLOSE_ID, NULL, NULL);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= INPUT_GRID_BASE_ID + 16 && id < INPUT_GRID_BASE_ID + 32) {
            selectedKey = id - (INPUT_GRID_BASE_ID + 16);
            SetWindowTextA(hWnd, ("Press new key for " + std::to_string(selectedKey)).c_str());
            SetFocus(hWnd);
            return TRUE;
        }
        if (id == INPUT_RESTORE_ID) {
            restoreDefaultKeymap();
            for (int i = 0; i < 16; ++i) {
                HWND hButton = GetDlgItem(hWnd, INPUT_GRID_BASE_ID + 16 + i);
                SetWindowTextA(hButton, getKeyName(keymap[i]).c_str());
            }
            for (int i = 0; i < 16; ++i) g_config.inputKeymap[i] = keymap[i];
            g_config.save(getConfigPath());
            return TRUE;
        }
        if (id == INPUT_CLOSE_ID) {
            DestroyWindow(hWnd);
            g_inputDlg = nullptr;
            return TRUE;
        }
        break;
    }
    case WM_KEYDOWN: {
        if (selectedKey >= 0 && selectedKey < 16) {
            SDL_Keycode sdlKey = 0;
            auto it = vkToSDLKey.find((UINT)wParam);
            if (it != vkToSDLKey.end()) {
                sdlKey = it->second;
            } else {
                sdlKey = (SDL_Keycode)wParam;
            }
            keymap[selectedKey] = sdlKey;
            HWND hButton = GetDlgItem(hWnd, INPUT_GRID_BASE_ID + 16 + selectedKey);
            SetWindowTextA(hButton, getKeyName(keymap[selectedKey]).c_str());
            SetWindowTextA(hWnd, "Remap CHIP-8 Keys");
            selectedKey = -1;
            for (int i = 0; i < 16; ++i) g_config.inputKeymap[i] = keymap[i];
            g_config.save(getConfigPath());
        }
        break;
    }
    case WM_DESTROY:
        g_inputDlg = nullptr;
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ShowInputRemapDialog(HWND parent) {
    if (g_inputDlg) {
        SetForegroundWindow(g_inputDlg);
        return;
    }
    static bool registered = false;
    if (!registered) {
        WNDCLASSA inputWc = {0};
        inputWc.lpfnWndProc = InputRemapDlgProc;
        inputWc.hInstance = GetModuleHandleA(NULL);
        inputWc.lpszClassName = "InputRemapWindow";
        inputWc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        inputWc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassA(&inputWc);
        registered = true;
    }
    g_inputDlg = CreateWindowExA(0, "InputRemapWindow", "Input Remap",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 650, 300, parent, NULL, GetModuleHandleA(NULL), NULL);
    if (g_inputDlg) {
        SetWindowTextA(g_inputDlg, "Remap CHIP-8 Keys");
        SetWindowPos(g_inputDlg, HWND_TOP, 0, 0, 650, 300, SWP_NOMOVE | SWP_SHOWWINDOW);
    }
}

void updateMenuBar() {
    if (!g_window) return;
    if (!g_hwnd) {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(g_window, &info)) {
            g_hwnd = info.info.win.window;
        }
    }
    HWND hwnd = g_hwnd;
    if (!hwnd) return;
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuA(hFileMenu, MF_STRING, 1001, "Open ROM\tCtrl+O");
    HMENU hRecentMenu = CreatePopupMenu();
    int id = 1010;
    for (const auto& path : recentROMs) {
        std::string label = path;
        if (label.size() > 40) label = "..." + label.substr(label.size() - 37);
        AppendMenuA(hRecentMenu, MF_STRING, id, label.c_str());
        ++id;
    }
    AppendMenuA(hFileMenu, MF_POPUP, (UINT_PTR)hRecentMenu, "Recent ROMs");
    AppendMenuA(hFileMenu, MF_STRING, 1003, "Close ROM\tCtrl+C");
    AppendMenuA(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hFileMenu, MF_STRING, 1004, "Exit\tESC");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    HMENU hOptionsMenu = CreatePopupMenu();
    const char* pauseLabel = (g_paused && *g_paused ? "Resume\tCtrl+P" : "Pause\tCtrl+P");
    AppendMenuA(hOptionsMenu, MF_STRING, 2001, pauseLabel);
    AppendMenuA(hOptionsMenu, MF_STRING, 2002, "Reset\tCtrl+R");
    AppendMenuA(hOptionsMenu, MF_STRING, 2004, "Input");
    AppendMenuA(hOptionsMenu, MF_STRING, 2003, "Audio");
    HMENU hModeMenu = CreatePopupMenu();
    UINT modeCheck = 0;
    if (g_cpu) {
        switch (g_cpu->getVariant()) {
            case Chip8CPU::Variant::CHIP8: modeCheck = 2201; break;
            case Chip8CPU::Variant::SCHIP: modeCheck = 2202; break;
            case Chip8CPU::Variant::XOCHIP: modeCheck = 2203; break;
        }
    }
    AppendMenuA(hModeMenu, MF_STRING | (modeCheck == 2201 ? MF_CHECKED : 0), 2201, "CHIP-8");
    AppendMenuA(hModeMenu, MF_STRING | (modeCheck == 2202 ? MF_CHECKED : 0), 2202, "SuperChip");
    AppendMenuA(hModeMenu, MF_STRING | (modeCheck == 2203 ? MF_CHECKED : 0), 2203, "XO-Chip");
    AppendMenuA(hOptionsMenu, MF_POPUP, (UINT_PTR)hModeMenu, "Mode\tF1");
    HMENU hScaleMenu = CreatePopupMenu();
    int scale = windowScale;
    AppendMenuA(hScaleMenu, MF_STRING | (scale == 5 ? MF_CHECKED : 0), 2101, "1x");
    AppendMenuA(hScaleMenu, MF_STRING | (scale == 10 ? MF_CHECKED : 0), 2102, "2x");
    AppendMenuA(hScaleMenu, MF_STRING | (scale == 15 ? MF_CHECKED : 0), 2103, "3x");
    AppendMenuA(hOptionsMenu, MF_POPUP, (UINT_PTR)hScaleMenu, "Window Scale\tF2");
    AppendMenuA(hOptionsMenu, MF_STRING, 2006, "Screenshot\tF3");
    HMENU hStatesMenu = CreatePopupMenu();
    AppendMenuA(hStatesMenu, MF_STRING, 2301, "Save State\tCtrl+S");
    AppendMenuA(hStatesMenu, MF_STRING, 2302, "Load State\tCtrl+L");
    AppendMenuA(hOptionsMenu, MF_POPUP, (UINT_PTR)hStatesMenu, "States");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hOptionsMenu, "Options");
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuA(hHelpMenu, MF_STRING, 3001, "About");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, "Help");
    SetMenu(hwnd, hMenuBar);
}

void resizeWindow(SDL_Window* window, const Chip8Display& display);
bool saveScreenshot(SDL_Window* window, SDL_Renderer* renderer);

std::string getStatesDir() {
    char exePath[1024] = {0};
#ifdef _WIN32
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    char* lastSlash = strrchr(exePath, '\\');
#else
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath)-1);
    if (len != -1) exePath[len] = '\0';
    char* lastSlash = strrchr(exePath, '/');
#endif
    if (lastSlash) *lastSlash = '\0';
    std::string statesDir = std::string(exePath) +
#ifdef _WIN32
        "\\states";
    _mkdir(statesDir.c_str());
#else
        "/states";
    mkdir(statesDir.c_str(), 0755);
#endif
    return statesDir;
}

std::string getStateSlotPath() {
    return getStatesDir() +
#ifdef _WIN32
        "\\slot1.ch8s";
#else
        "/slot1.ch8s";
#endif
}

static void SDLCALL menuHandler(void* userdata, void* hWnd, unsigned int message, Uint64 wParam, Sint64 lParam) {
    if (message == WM_COMMAND) {
        UINT cmd = LOWORD(wParam);
        switch (cmd) {
            case 1001: { /* Open ROM (Ctrl+O) */
                std::string romPath = openFileDialog();
                if (!romPath.empty() && g_loadROM && *g_loadROM) {
                    (*g_loadROM)(romPath);
                    updateMenuBar();
                }
                break;
            }
            // Recent ROMs: 1010, 1011, 1012, ...
            case 1010: case 1011: case 1012: case 1013: case 1014: {
                int idx = cmd - 1010;
                if (idx >= 0 && idx < (int)recentROMs.size()) {
                    std::string romPath = recentROMs[idx];
                    if (!romPath.empty() && g_loadROM && *g_loadROM) {
                        (*g_loadROM)(romPath);
                        updateMenuBar();
                        g_config.recentROMs.clear();
                        for (const auto& rom : recentROMs) g_config.recentROMs.push_back(rom);
                        g_config.save(getConfigPath());
                    }
                }
                break;
            }
            case 1002: /* Recent ROM */

                break;
            case 1003: { /* Close ROM (Ctrl+C) */
                if (g_romLoaded && g_currentRomPath && g_currentRomData && g_paused && g_cpu && g_renderer) {
                    *g_romLoaded = false;
                    g_currentRomPath->clear();
                    g_currentRomData->clear();
                    *g_paused = false;
                    new (g_cpu) Chip8CPU(Chip8CPU::Variant::CHIP8); 
                    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
                    SDL_RenderClear(g_renderer);
                    SDL_RenderPresent(g_renderer);
                }
                break;
            }
            case 1004: /* Exit (ESC) */
                PostQuitMessage(0);
                break;
            case 2001: /* Pause/Resume (Ctrl+P) */
                if (g_paused) {
                    *g_paused = !(*g_paused);
                    menuPaused = *g_paused;
                    HMENU hMenuBar = GetMenu((HWND)hWnd);
                    HMENU hOptionsMenu = GetSubMenu(hMenuBar, 1);
                    ModifyMenuA(hOptionsMenu, 0, MF_BYPOSITION | MF_STRING, 2001, (*g_paused ? "Resume\tCtrl+P" : "Pause\tCtrl+P"));
                }
                break;
            case 2002: { /* Reset (Ctrl+R) */
                if (g_romLoaded && *g_romLoaded && g_cpu && g_currentRomData && g_lastMode && g_window) {
                    new (g_cpu) Chip8CPU(g_cpu->getVariant());
                    if (g_cpu->getVariant() == Chip8CPU::Variant::CHIP8) {
                        g_cpu->setQuirks({true, true, false});
                    } else if (g_cpu->getVariant() == Chip8CPU::Variant::SCHIP) {
                        g_cpu->setQuirks({false, false, true});
                    } else {
                        g_cpu->setQuirks({true, true, false});
                    }
                    g_cpu->memory().loadROM(*g_currentRomData);
                    resizeWindow(g_window, g_cpu->display());
                    *g_lastMode = g_cpu->display().getMode();
                    if (g_paused) *g_paused = false;
                    menuPaused = false;
                }
                break;
            }
            case 2201: /* Mode: CHIP-8 (F1) */
                if (g_cpu && g_currentRomData && g_lastMode && g_window) {
                    new (g_cpu) Chip8CPU(Chip8CPU::Variant::CHIP8);
                    g_cpu->setQuirks({true, true, false});
                    if (g_currentRomData && !g_currentRomData->empty())
                        g_cpu->memory().loadROM(*g_currentRomData);
                    resizeWindow(g_window, g_cpu->display());
                    *g_lastMode = g_cpu->display().getMode();
                    if (g_paused) *g_paused = false;
                    menuPaused = false;
                    updateMenuBar();
                }
                break;
            case 2202: /* Mode: SuperChip (F1) */
                if (g_cpu && g_currentRomData && g_lastMode && g_window) {
                    new (g_cpu) Chip8CPU(Chip8CPU::Variant::SCHIP);
                    g_cpu->setQuirks({false, false, true});
                    if (g_currentRomData && !g_currentRomData->empty())
                        g_cpu->memory().loadROM(*g_currentRomData);
                    resizeWindow(g_window, g_cpu->display());
                    *g_lastMode = g_cpu->display().getMode();
                    if (g_paused) *g_paused = false;
                    menuPaused = false;
                    updateMenuBar();
                }
                break;
            case 2203: /* Mode: XO-Chip (F1) */
                if (g_cpu && g_currentRomData && g_lastMode && g_window) {
                    new (g_cpu) Chip8CPU(Chip8CPU::Variant::XOCHIP);
                    g_cpu->setQuirks({true, true, false});
                    if (g_currentRomData && !g_currentRomData->empty())
                        g_cpu->memory().loadROM(*g_currentRomData);
                    resizeWindow(g_window, g_cpu->display());
                    *g_lastMode = g_cpu->display().getMode();
                    if (g_paused) *g_paused = false;
                    menuPaused = false;
                    updateMenuBar();
                }
                break;
            case 2101: /* Window Scale 1x (F2) */
                if (g_window && g_cpu) {
                    windowScale = 5;
                    SDL_SetWindowSize(g_window, g_cpu->display().width() * windowScale, g_cpu->display().height() * windowScale);
                    updateMenuBar();
                }
                break;
            case 2102: /* Window Scale 2x (F2) */
                if (g_window && g_cpu) {
                    windowScale = 10;
                    SDL_SetWindowSize(g_window, g_cpu->display().width() * windowScale, g_cpu->display().height() * windowScale);
                    updateMenuBar();
                }
                break;
            case 2103: /* Window Scale 3x (F2) */
                if (g_window && g_cpu) {
                    windowScale = 15;
                    SDL_SetWindowSize(g_window, g_cpu->display().width() * windowScale, g_cpu->display().height() * windowScale);
                    updateMenuBar();
                }
                break;
            case 2003: /* Audio */
                if (g_hwnd) ShowAudioDialog(g_hwnd);
                break;
            case 2004: /* Input */
                if (g_hwnd) ShowInputRemapDialog(g_hwnd);
                break;
            case 2006: /* Screenshot (F3) */
                if (g_window && g_renderer) {
                    bool ok = saveScreenshot(g_window, g_renderer);
                    HWND hwnd = nullptr;
                    SDL_SysWMinfo info;
                    SDL_VERSION(&info.version);
                    if (SDL_GetWindowWMInfo(g_window, &info)) {
                        hwnd = info.info.win.window;
                    }
                    MessageBoxA(hwnd, ok ? "Screenshot saved!" : "Screenshot failed!", "Screenshot", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONERROR));
                }
                break;
            case 2301: /* Save State (Ctrl+S) */
                if (g_cpu) {
                    bool ok = g_cpu->saveState(getStateSlotPath());
                    MessageBoxA((HWND)hWnd, ok ? "State saved!" : "Save failed!", "Save State", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONERROR));
                }
                break;
            case 2302: /* Load State (Ctrl+L) */
                if (g_cpu) {
                    bool ok = g_cpu->loadState(getStateSlotPath());
                    MessageBoxA((HWND)hWnd, ok ? "State loaded!" : "Load failed!", "Load State", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONERROR));
                }
                break;
            case 3001: /* About */
                MessageBoxA((HWND)hWnd,
                    "CHIP8CHAPA\nA nice CHIP-8 emulator.\n\n(c) CHIP8CHAPA 2025",
                    "About CHIP8CHAPA",
                    MB_OK | MB_ICONINFORMATION);
                break;
            default:
                break;
        }
    }
}
#endif

#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <SDL_surface.h>
#include <SDL_render.h>
bool saveScreenshot(SDL_Window* window, SDL_Renderer* renderer) {
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) {
        return false;
    }
    if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA32, surf->pixels, surf->pitch) != 0) {
        SDL_FreeSurface(surf);
        return false;
    }
    char exePath[1024] = {0};
#ifdef _WIN32
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    char* lastSlash = strrchr(exePath, '\\');
#else
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath)-1);
    if (len != -1) exePath[len] = '\0';
    char* lastSlash = strrchr(exePath, '/');
#endif
    if (lastSlash) *lastSlash = '\0';
    std::string screenshotsDir = std::string(exePath) +
#ifdef _WIN32
        "\\screenshots";
    _mkdir(screenshotsDir.c_str());
#else
        "/screenshots";
    mkdir(screenshotsDir.c_str(), 0755);
#endif
    std::ostringstream oss;
    std::time_t t = std::time(nullptr);
    oss << screenshotsDir <<
#ifdef _WIN32
        "\\screenshot_";
#else
        "/screenshot_";
#endif
    oss << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".png";
    std::string filename = oss.str();
    if (!surf->pixels) {
        SDL_FreeSurface(surf);
        return false;
    }
    int ok = stbi_write_png(filename.c_str(), w, h, 4, surf->pixels, surf->pitch);
    SDL_FreeSurface(surf);
    return ok != 0;
}

void setWindowTitle(SDL_Window* window, const std::string& romPath) {
    std::string title = "CHIP8CHAPA";
    if (!romPath.empty()) {
        size_t slash = romPath.find_last_of("/\\");
        std::string fname = (slash != std::string::npos) ? romPath.substr(slash + 1) : romPath;
        size_t dot = fname.find_last_of('.');
        if (dot != std::string::npos) fname = fname.substr(0, dot);
        title += " - " + fname;
    }
    SDL_SetWindowTitle(window, title.c_str());
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    g_config.load(getConfigPath());
    windowScale = g_config.windowScale;
    for (size_t i = 0; i < g_config.recentROMs.size(); ++i) {
        if (i < MAX_RECENT_ROMS) recentROMs.push_back(g_config.recentROMs[i]);
    }
    audioMuted = g_config.audioMuted;
    audioVolume = g_config.audioVolume;
    for (int i = 0; i < 16; ++i) keymap[i] = g_config.inputKeymap[i];

    int initW = Chip8Display::LOWRES_WIDTH * windowScale;
    int initH = Chip8Display::LOWRES_HEIGHT * windowScale;
    SDL_Window* window = SDL_CreateWindow("CHIP8CHAPA", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, initW, initH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        std::cerr << "SDL_CreateWindow/Renderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    Chip8CPU::Variant initialVariant = Chip8CPU::Variant::CHIP8;
    if (g_config.mode == 1) initialVariant = Chip8CPU::Variant::SCHIP;
    else if (g_config.mode == 2) initialVariant = Chip8CPU::Variant::XOCHIP;
    Chip8CPU cpu(initialVariant);
    if (initialVariant == Chip8CPU::Variant::CHIP8) {
        cpu.setQuirks({true, true, false});
    } else if (initialVariant == Chip8CPU::Variant::SCHIP) {
        cpu.setQuirks({false, false, true});
    } else {
        cpu.setQuirks({true, true, false});
    }
    auto lastMode = cpu.display().getMode();
#ifdef _WIN32
    g_window = window;
    g_renderer = renderer;
#endif
    std::string currentRomPath;
    std::vector<uint8_t> currentRomData;
    bool romLoaded = false;
    bool running = true;
    bool paused = false;
    bool pausedByMenu = false;
    bool wasPausedBeforeMenu = false;
    bool lastPaused = false;
#ifdef _WIN32
    g_romLoaded = &romLoaded;
    g_currentRomPath = &currentRomPath;
    g_currentRomData = &currentRomData;
    g_paused = &paused;
    g_cpu = &cpu;
    g_lastMode = &lastMode;
    static std::function<bool(const std::string&)> loadROM = [&](const std::string& romPath) -> bool {
        std::ifstream rom(romPath, std::ios::binary);
        if (!rom) {
            std::cerr << "Failed to open ROM file: " << romPath << std::endl;
            return false;
        }
        std::vector<uint8_t> romData((std::istreambuf_iterator<char>(rom)), std::istreambuf_iterator<char>());
        new (&cpu) Chip8CPU(Chip8CPU::Variant::CHIP8); 
        cpu.setQuirks({true, true, false});
        cpu.memory().loadROM(romData);
        resizeWindow(window, cpu.display());
        lastMode = cpu.display().getMode();
        currentRomPath = romPath;
        currentRomData = romData;
        romLoaded = true;
        paused = false;
        addRecentROM(romPath);
        updateMenuBar();
        setWindowTitle(window, romPath);
        g_config.recentROMs.clear();
        for (const auto& rom : recentROMs) g_config.recentROMs.push_back(rom);
        g_config.save(getConfigPath());
        return true;
    };
    g_loadROM = &loadROM;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    HWND hwnd = nullptr;
    if (SDL_GetWindowWMInfo(window, &info)) {
        hwnd = info.info.win.window;
    }
    if (hwnd) {
        updateMenuBar();
    }
    SDL_SetWindowsMessageHook(menuHandler, nullptr);
#endif

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    double instrDelay;
    if (cpu.getVariant() == Chip8CPU::Variant::CHIP8) {
        instrDelay = 1.0 / 700.0;
    } else if (cpu.getVariant() == Chip8CPU::Variant::SCHIP) {
        instrDelay = 1.0 / 1000.0;
    } else { 
        instrDelay = 1.0 / 2000.0;
    }
    double timerDelay = 1.0 / TIMER_HZ;
    double instrAccum = 0.0, timerAccum = 0.0;
    auto lastTimer = std::chrono::high_resolution_clock::now();
    auto lastInstr = lastTimer;

    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastInstr).count();
        lastInstr = now;
        instrAccum += elapsed;
        timerAccum += elapsed;

        int dispW = cpu.display().width();
        int dispH = cpu.display().height();
        float aspect = static_cast<float>(dispW) / dispH;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
                int winW = e.window.data1;
                int winH = e.window.data2;
                float newAspect = static_cast<float>(winW) / winH;
                if (std::abs(newAspect - aspect) > 0.01f) {
                    if (newAspect > aspect) {
                        winW = static_cast<int>(winH * aspect);
                    } else {
                        winH = static_cast<int>(winW / aspect);
                    }
                    SDL_SetWindowSize(window, winW, winH);
                }
            }
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode key = e.key.keysym.sym;
                SDL_Keymod mod = SDL_GetModState();
                if (key == SDLK_ESCAPE) running = false;
                if (key == SDLK_o && (mod & KMOD_CTRL)) {
                    std::string romPath = openFileDialog();
                    if (!romPath.empty()) loadROM(romPath);
                }
                if (key == SDLK_c && (mod & KMOD_CTRL)) {
                    romLoaded = false;
                    currentRomPath.clear();
                    currentRomData.clear();
                    paused = false;
                    new (&cpu) Chip8CPU(Chip8CPU::Variant::CHIP8);
                    cpu.setQuirks({true, true, false});
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderClear(renderer);
                    SDL_RenderPresent(renderer);
                }
                if (key == SDLK_p && (mod & KMOD_CTRL)) {
                    paused = !paused;
#ifdef _WIN32
                    updateMenuBar();
#endif
                }
                if (key == SDLK_r && (mod & KMOD_CTRL) && romLoaded) {
                    new (&cpu) Chip8CPU(cpu.getVariant());
                    if (cpu.getVariant() == Chip8CPU::Variant::CHIP8) {
                        cpu.setQuirks({true, true, false});
                    } else if (cpu.getVariant() == Chip8CPU::Variant::SCHIP) {
                        cpu.setQuirks({false, false, true});
                    } else {
                        cpu.setQuirks({true, true, false});
                    }
                    cpu.memory().loadROM(currentRomData);
                    resizeWindow(window, cpu.display());
                    lastMode = cpu.display().getMode();
                    paused = false;
                }
                if (key == SDLK_F1) {
                    Chip8CPU::Variant nextVariant = Chip8CPU::Variant::CHIP8;
                    switch (cpu.getVariant()) {
                        case Chip8CPU::Variant::CHIP8: nextVariant = Chip8CPU::Variant::SCHIP; break;
                        case Chip8CPU::Variant::SCHIP: nextVariant = Chip8CPU::Variant::XOCHIP; break;
                        case Chip8CPU::Variant::XOCHIP: nextVariant = Chip8CPU::Variant::CHIP8; break;
                    }
                    new (&cpu) Chip8CPU(nextVariant);
                    if (nextVariant == Chip8CPU::Variant::CHIP8) {
                        cpu.setQuirks({true, true, false});
                    } else if (nextVariant == Chip8CPU::Variant::SCHIP) {
                        cpu.setQuirks({false, false, true});
                    } else {
                        cpu.setQuirks({true, true, false});
                    }
                    if (romLoaded) {
                        cpu.memory().loadROM(currentRomData);
                    }
                    cpu.display().setColorMode(Chip8Display::ColorMode::Mono);
                    resizeWindow(window, cpu.display());
                    lastMode = cpu.display().getMode();
                    paused = false;
#ifdef _WIN32
                    updateMenuBar();
#endif
                }
                if (key == SDLK_F2) {
                    if (windowScale == 5) windowScale = 10;
                    else if (windowScale == 10) windowScale = 15;
                    else windowScale = 5;
                    SDL_SetWindowSize(window, cpu.display().width() * windowScale, cpu.display().height() * windowScale);
#ifdef _WIN32
                    updateMenuBar();
#endif
                }
                if (key == SDLK_s && (mod & KMOD_CTRL)) {
                    if (g_cpu) {
                        bool ok = g_cpu->saveState(getStateSlotPath());
#ifdef _WIN32
                        HWND hwnd = nullptr;
                        SDL_SysWMinfo info;
                        SDL_VERSION(&info.version);
                        if (SDL_GetWindowWMInfo(window, &info)) {
                            hwnd = info.info.win.window;
                        }
                        MessageBoxA(hwnd, ok ? "State saved!" : "Save failed!", "Save State", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONERROR));
#else
                        printf("State %s\n", ok ? "saved!" : "save failed!");
#endif
                    }
                }
                if (key == SDLK_l && (mod & KMOD_CTRL)) {
                    if (g_cpu) {
                        bool ok = g_cpu->loadState(getStateSlotPath());
#ifdef _WIN32
                        HWND hwnd = nullptr;
                        SDL_SysWMinfo info;
                        SDL_VERSION(&info.version);
                        if (SDL_GetWindowWMInfo(window, &info)) {
                            hwnd = info.info.win.window;
                        }
                        MessageBoxA(hwnd, ok ? "State loaded!" : "Load failed!", "Load State", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONERROR));
#else
                        printf("State %s\n", ok ? "loaded!" : "load failed!");
#endif
                    }
                }
                if (key == SDLK_F3) {
#ifdef _WIN32
                    if (window && renderer) {
                        bool ok = saveScreenshot(window, renderer);
                        HWND hwnd = nullptr;
                        SDL_SysWMinfo info;
                        SDL_VERSION(&info.version);
                        if (SDL_GetWindowWMInfo(window, &info)) {
                            hwnd = info.info.win.window;
                        }
                        MessageBoxA(hwnd, ok ? "Screenshot saved!" : "Screenshot failed!", "Screenshot", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONERROR));
                    }
#else
                    if (window && renderer) {
                        bool ok = saveScreenshot(window, renderer);
                        printf("Screenshot %s\n", ok ? "saved!" : "failed!");
                    }
#endif
                }
            }
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                bool pressed = (e.type == SDL_KEYDOWN);
                for (int i = 0; i < 16; ++i) {
                    if (e.key.keysym.sym == keymap[i]) {
                        cpu.input().setKey(i, pressed);
                    }
                }
            }
        }

#ifdef _WIN32
        static HWND hwnd = nullptr;
        if (!hwnd) {
            SDL_SysWMinfo info;
            SDL_VERSION(&info.version);
            if (SDL_GetWindowWMInfo(window, &info)) {
                hwnd = info.info.win.window;
            }
        }
        if (hwnd) {
            HMENU hMenuBar = GetMenu(hwnd);
            if (hMenuBar) {
                bool menuActive = false;
                int menuCount = GetMenuItemCount(hMenuBar);
                for (int i = 0; i < menuCount; ++i) {
                    UINT state = GetMenuState(hMenuBar, i, MF_BYPOSITION);
                    if (state & MF_POPUP) {
                        HMENU hSub = GetSubMenu(hMenuBar, i);
                        if (hSub && (GetMenuState(hSub, 0, MF_BYPOSITION) & MF_HILITE)) {
                            menuActive = true;
                            break;
                        }
                    }
                }
                if (menuActive && !pausedByMenu) {
                    wasPausedBeforeMenu = paused;
                    paused = true;
                    pausedByMenu = true;
                } else if (!menuActive && pausedByMenu) {
                    paused = wasPausedBeforeMenu;
                    pausedByMenu = false;
                }
            }
        }
#endif
        if (paused != lastPaused) {
            if (paused) {
                cpu.sound().stop();
                cpu.sound().forceSilence();
                cpu.timers().setSound(0);
            } else {
                if (cpu.timers().getSound() > 0) {
                    cpu.sound().start();
                }
            }
            lastPaused = paused;
        }

        if (romLoaded && !paused) {
            while (instrAccum >= instrDelay) {
                cpu.step();
                instrAccum -= instrDelay;
            }
            while (timerAccum >= timerDelay) {
                cpu.timers().tick();
                timerAccum -= timerDelay;
            }
            cpu.sound().update();
            if (cpu.display().getMode() != lastMode) {
                resizeWindow(window, cpu.display());
                lastMode = cpu.display().getMode();
            }
            renderDisplay(window, renderer, cpu.display());
        } else if (!romLoaded) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    g_config.recentROMs.clear();
    for (const auto& rom : recentROMs) g_config.recentROMs.push_back(rom);
    g_config.audioMuted = audioMuted;
    g_config.audioVolume = audioVolume;
    for (int i = 0; i < 16; ++i) g_config.inputKeymap[i] = keymap[i];
    g_config.windowScale = windowScale;
    if (cpu.getVariant() == Chip8CPU::Variant::CHIP8) g_config.mode = 0;
    else if (cpu.getVariant() == Chip8CPU::Variant::SCHIP) g_config.mode = 1;
    else g_config.mode = 2;
    g_config.save(getConfigPath());

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
} 