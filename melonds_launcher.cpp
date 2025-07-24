/*
    melonDS Minimal SDL Frontend
    Copyright (C) 2024 melonDS team
    
    This file is part of melonDS.
    
    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.
    
    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <SDL2/SDL.h>
#include <string>
#include <memory>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <vector>

// melonDS includes
#include "NDS.h"
#include "DSi.h"
#include "NDSCart.h"
#include "GBACart.h"
#include "GPU.h"
#include "GPU3D_Soft.h"
#include "SPU.h"
#include "Platform.h"
#include "Args.h"

using namespace melonDS;

class MinimalFrontend
{
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* screenTexture;
    
    std::unique_ptr<NDS> nds;
    
    bool running;
    bool frameLimiter;
    int targetFPS;
    
    u32 inputMask;
    
    // FPS counter
    bool showFPS;
    int frameCount;
    std::chrono::steady_clock::time_point lastFPSUpdate;
    float currentFPS;
    
    // Input device
    int inputFd;
    
    // Screen dimensions
    static const int SCREEN_WIDTH = 256;
    static const int SCREEN_HEIGHT = 192 * 2; // Both screens
    
    // Audio
    SDL_AudioDeviceID audioDevice;
    
    // DS button mapping - maps Linux input codes to DS button bits
    struct ButtonMapping {
        int linuxCode;
        int dsButton;
        const char* name;
    };
    
    ButtonMapping buttonMap[17] = {
        // Common gamepad button codes - adjust these based on your handheld
        {BTN_A, 0, "A"},              // 304 - A button
        {BTN_B, 1, "B"},              // 305 - B button  
        {BTN_SELECT, 2, "Select"},    // 314 - Select
        {BTN_START, 3, "Start"},      // 315 - Start
        {BTN_DPAD_RIGHT, 4, "Right"}, // 307 - D-pad Right
        {BTN_DPAD_LEFT, 5, "Left"},   // 306 - D-pad Left
        {BTN_DPAD_UP, 6, "Up"},       // 308 - D-pad Up
        {BTN_DPAD_DOWN, 7, "Down"},   // 309 - D-pad Down
        {BTN_TL, 8, "L"},             // 310 - L shoulder
        {BTN_TR, 9, "R"},             // 311 - R shoulder
        {BTN_X, 10, "X"},             // 307 - X button
        {BTN_Y, 11, "Y"},             // 308 - Y button
        // Alternative mappings for different handhelds
        {KEY_ESC, -1, "Exit"},        // ESC to exit
        {KEY_F1, -1, "FPS_Toggle"},   // F1 to toggle FPS
        {304, 0, "A_Alt"},            // Alternative A
        {305, 1, "B_Alt"},            // Alternative B
        {0, -1, ""}                   // End marker
    };
    
    // D-pad state for HAT events
    int hatX, hatY;
    
public:
    MinimalFrontend() : window(nullptr), renderer(nullptr), screenTexture(nullptr), 
                       running(false), frameLimiter(true), targetFPS(30), inputMask(0xFFF), 
                       audioDevice(0), inputFd(-1), hatX(0), hatY(0), showFPS(true), frameCount(0), currentFPS(0.0f) {
        lastFPSUpdate = std::chrono::steady_clock::now();
    }
    
    ~MinimalFrontend() {
        cleanup();
    }
    
    bool initialize() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
            std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Open input device
        inputFd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
        if (inputFd < 0) {
            std::cerr << "Failed to open /dev/input/event1. You may need to run as root or adjust permissions." << std::endl;
            std::cerr << "Try: sudo chmod 644 /dev/input/event1" << std::endl;
            return false;
        }
        
        std::cout << "Successfully opened /dev/input/event1 for handheld input" << std::endl;
        
        // Create window  
        window = SDL_CreateWindow("melonDS",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, // 2x scale
                                SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create screen texture
        screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX8888,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        SCREEN_WIDTH, SCREEN_HEIGHT);
        if (!screenTexture) {
            std::cerr << "Screen texture creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize audio
        SDL_AudioSpec audioSpec;
        audioSpec.freq = 32000;
        audioSpec.format = AUDIO_S16LSB;
        audioSpec.channels = 2;
        audioSpec.samples = 512;
        audioSpec.callback = nullptr;
        
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, 0);
        if (audioDevice == 0) {
            std::cerr << "Audio device open failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        SDL_PauseAudioDevice(audioDevice, 0);
        
        return true;
    }
    
    // Load ARM9 BIOS (4KB)
    std::unique_ptr<ARM9BIOSImage> loadBIOS9(const std::string& biosPath) {
        // Try common ARM9 BIOS filenames
        std::vector<std::string> filenames = {
            "biosnds9.bin",
            "bios9.bin", 
            "nds_bios_arm9.bin",
            "arm9.bin"
        };
        
        for (const auto& filename : filenames) {
            std::string fullPath = biosPath + filename;
            Platform::FileHandle* file = Platform::OpenFile(fullPath, Platform::FileMode::Read);
            if (file) {
                long size = Platform::FileLength(file);
                if (size == ARM9BIOSSize) {
                    auto bios = std::make_unique<ARM9BIOSImage>();
                    Platform::FileRewind(file);
                    size_t bytesRead = Platform::FileRead(bios->data(), ARM9BIOSSize, 1, file);
                    Platform::CloseFile(file);
                    
                    if (bytesRead == 1) {
                        std::cout << "Loaded ARM9 BIOS: " << fullPath << std::endl;
                        return bios;
                    }
                }
                Platform::CloseFile(file);
            }
        }
        return nullptr;
    }
    
    // Load ARM7 BIOS (16KB)
    std::unique_ptr<ARM7BIOSImage> loadBIOS7(const std::string& biosPath) {
        // Try common ARM7 BIOS filenames
        std::vector<std::string> filenames = {
            "biosnds7.bin",
            "bios7.bin",
            "nds_bios_arm7.bin", 
            "arm7.bin"
        };
        
        for (const auto& filename : filenames) {
            std::string fullPath = biosPath + filename;
            Platform::FileHandle* file = Platform::OpenFile(fullPath, Platform::FileMode::Read);
            if (file) {
                long size = Platform::FileLength(file);
                if (size == ARM7BIOSSize) {
                    auto bios = std::make_unique<ARM7BIOSImage>();
                    Platform::FileRewind(file);
                    size_t bytesRead = Platform::FileRead(bios->data(), ARM7BIOSSize, 1, file);
                    Platform::CloseFile(file);
                    
                    if (bytesRead == 1) {
                        std::cout << "Loaded ARM7 BIOS: " << fullPath << std::endl;
                        return bios;
                    }
                }
                Platform::CloseFile(file);
            }
        }
        return nullptr;
    }
    
    bool loadROM(const std::string& romPath) {
        std::cout << "Loading ROM: " << romPath << std::endl;
        
        // Create NDS arguments with BIOS loading
        NDSArgs args = {};
        
        // Try to load real BIOS files from MUOS bios directory
        const std::string biosPath = "/mnt/sdcard/MUOS/bios/";
        std::unique_ptr<ARM9BIOSImage> arm9bios = loadBIOS9(biosPath);
        std::unique_ptr<ARM7BIOSImage> arm7bios = loadBIOS7(biosPath);
        
        if (arm9bios) {
            args.ARM9BIOS = std::move(arm9bios);
            std::cout << "Using real ARM9 BIOS" << std::endl;
        } else {
            std::cout << "Using FreeBIOS for ARM9 (real BIOS not found)" << std::endl;
        }
        
        if (arm7bios) {
            args.ARM7BIOS = std::move(arm7bios);
            std::cout << "Using real ARM7 BIOS" << std::endl;
        } else {
            std::cout << "Using FreeBIOS for ARM7 (real BIOS not found)" << std::endl;
        }
        
        // Create NDS console
        nds = std::make_unique<NDS>(std::move(args), this);
        
        // Load ROM file
        Platform::FileHandle* romFile = Platform::OpenFile(romPath, Platform::FileMode::Read);
        if (!romFile) {
            std::cerr << "Failed to open ROM file: " << romPath << std::endl;
            return false;
        }
        
        long romSize = Platform::FileLength(romFile);
        if (romSize <= 0 || romSize > 0x40000000) {
            std::cerr << "Invalid ROM size: " << romSize << std::endl;
            Platform::CloseFile(romFile);
            return false;
        }
        
        Platform::FileRewind(romFile);
        auto romData = std::make_unique<u8[]>(romSize);
        size_t bytesRead = Platform::FileRead(romData.get(), romSize, 1, romFile);
        Platform::CloseFile(romFile);
        
        if (bytesRead != 1) {
            std::cerr << "Failed to read ROM data" << std::endl;
            return false;
        }
        
        // Parse and load the cart
        NDSCart::NDSCartArgs cartArgs = {};
        auto cart = NDSCart::ParseROM(std::move(romData), romSize, this, std::move(cartArgs));
        if (!cart) {
            std::cerr << "Failed to parse ROM" << std::endl;
            return false;
        }
        
        nds->SetNDSCart(std::move(cart));
        nds->Reset();
        
        // Enable direct boot (skip BIOS screens)
        size_t lastSlash = romPath.find_last_of("/\\");
        std::string romName = (lastSlash != std::string::npos) ? romPath.substr(lastSlash + 1) : romPath;
        nds->SetupDirectBoot(romName);
        
        nds->Start();
        
        std::cout << "ROM loaded successfully!" << std::endl;
        return true;
    }
    
    void handleInput() {
        // Read Linux input events from /dev/input/event1
        struct input_event ev;
        
        while (read(inputFd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_KEY) {
                bool pressed = (ev.value == 1);
                bool released = (ev.value == 0);
                
                if (!pressed && !released) continue; // Skip repeat events
                
                // Handle exit key
                if (ev.code == KEY_ESC && pressed) {
                    running = false;
                    return;
                }
                
                // Toggle FPS display with F1
                if (ev.code == KEY_F1 && pressed) {
                    showFPS = !showFPS;
                    if (showFPS) {
                        std::cout << "FPS display enabled" << std::endl;
                    } else {
                        std::cout << "FPS display disabled" << std::endl;
                    }
                    return;
                }
                
                // Map button to DS input
                for (const auto& mapping : buttonMap) {
                    if (mapping.linuxCode == (int)ev.code && mapping.dsButton >= 0) {
                        if (pressed) {
                            inputMask &= ~(1 << mapping.dsButton);
                            std::cout << "Button pressed: " << mapping.name << " (code " << ev.code << ")" << std::endl;
                        } else {
                            inputMask |= (1 << mapping.dsButton);
                            std::cout << "Button released: " << mapping.name << " (code " << ev.code << ")" << std::endl;
                        }
                        break;
                    }
                }
            }
            else if (ev.type == EV_ABS) {
                // Handle analog hat/d-pad
                if (ev.code == ABS_HAT0X) {
                    // Update D-pad left/right state
                    if (hatX != ev.value) {
                        // Clear previous left/right state
                        inputMask |= (1 << 4) | (1 << 5); // Clear right and left
                        
                        hatX = ev.value;
                        if (hatX < 0) {
                            inputMask &= ~(1 << 5); // Left pressed
                            std::cout << "D-pad: Left" << std::endl;
                        } else if (hatX > 0) {
                            inputMask &= ~(1 << 4); // Right pressed  
                            std::cout << "D-pad: Right" << std::endl;
                        }
                    }
                }
                else if (ev.code == ABS_HAT0Y) {
                    // Update D-pad up/down state
                    if (hatY != ev.value) {
                        // Clear previous up/down state
                        inputMask |= (1 << 6) | (1 << 7); // Clear up and down
                        
                        hatY = ev.value;
                        if (hatY < 0) {
                            inputMask &= ~(1 << 6); // Up pressed
                            std::cout << "D-pad: Up" << std::endl;
                        } else if (hatY > 0) {
                            inputMask &= ~(1 << 7); // Down pressed
                            std::cout << "D-pad: Down" << std::endl;
                        }
                    }
                }
            }
        }
        
        // Also handle SDL window events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
    }
    
    void debugInputMapping() {
        bool debugging = true;
        struct input_event ev;
        
        while (debugging) {
            while (read(inputFd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    if (ev.value == 1) { // Only show press events
                        std::cout << "Key pressed: code=" << ev.code << " (0x" << std::hex << ev.code << std::dec << ")";
                        
                        // Show if we have a mapping for this code
                        for (const auto& mapping : buttonMap) {
                            if (mapping.linuxCode == (int)ev.code) {
                                std::cout << " -> " << mapping.name << " (DS button " << mapping.dsButton << ")";
                                break;
                            }
                        }
                        std::cout << std::endl;
                        
                        // Exit debug on ESC
                        if (ev.code == KEY_ESC) {
                            debugging = false;
                            break;
                        }
                    }
                }
                else if (ev.type == EV_ABS) {
                    if (ev.code == ABS_HAT0X || ev.code == ABS_HAT0Y) {
                        std::cout << "D-pad axis: code=" << ev.code;
                        if (ev.code == ABS_HAT0X) {
                            std::cout << " (X-axis)";
                        } else {
                            std::cout << " (Y-axis)";
                        }
                        std::cout << " value=" << ev.value;
                        if (ev.value < 0) std::cout << " (negative/up-left)";
                        else if (ev.value > 0) std::cout << " (positive/down-right)";
                        else std::cout << " (center)";
                        std::cout << std::endl;
                    }
                }
            }
            
            // Small delay to prevent busy waiting
            usleep(10000); // 10ms
        }
        
        std::cout << "Exiting debug mode..." << std::endl;
    }
    
    void runFrame() {
        if (!nds) return;
        
        // Update input
        nds->SetKeyMask(inputMask);
        
        // Run one frame of emulation
        nds->RunFrame();
        
        // Get screen data
        u32* frontBuffer = nds->GPU.Framebuffer[nds->GPU.FrontBuffer][0].get();
        u32* backBuffer = nds->GPU.Framebuffer[nds->GPU.FrontBuffer][1].get();
        
        // Update screen texture
        void* pixels;
        int pitch;
        if (SDL_LockTexture(screenTexture, nullptr, &pixels, &pitch) == 0) {
            u32* texPixels = static_cast<u32*>(pixels);
            
            // Copy top screen
            for (int y = 0; y < 192; y++) {
                memcpy(&texPixels[y * SCREEN_WIDTH], &frontBuffer[y * SCREEN_WIDTH], SCREEN_WIDTH * 4);
            }
            
            // Copy bottom screen
            for (int y = 0; y < 192; y++) {
                memcpy(&texPixels[(y + 192) * SCREEN_WIDTH], &backBuffer[y * SCREEN_WIDTH], SCREEN_WIDTH * 4);
            }
            
            SDL_UnlockTexture(screenTexture);
        }
        
        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screenTexture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        
        // FPS calculation
        frameCount++;
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFPSUpdate);
        
        if (elapsed.count() >= 1000) { // Update FPS every second
            currentFPS = (float)frameCount / (elapsed.count() / 1000.0f);
            if (showFPS) {
                std::cout << "FPS: " << std::fixed << std::setprecision(1) << currentFPS << std::endl;
            }
            frameCount = 0;
            lastFPSUpdate = currentTime;
        }
        
        // Frame limiting
        if (frameLimiter) {
            static auto lastFrame = std::chrono::steady_clock::now();
            auto targetTime = std::chrono::milliseconds(1000 / targetFPS);
            auto frameElapsed = std::chrono::steady_clock::now() - lastFrame;
            
            if (frameElapsed < targetTime) {
                SDL_Delay(std::chrono::duration_cast<std::chrono::milliseconds>(targetTime - frameElapsed).count());
            }
            lastFrame = std::chrono::steady_clock::now();
        }
    }
    
    void run() {
        running = true;
        std::cout << "Starting emulation loop..." << std::endl;
        std::cout << "Reading input from handheld physical buttons via /dev/input/event1" << std::endl;
        std::cout << "Button mappings will be detected and displayed when pressed." << std::endl;
        std::cout << "Press F1 to toggle FPS display (currently " << (showFPS ? "enabled" : "disabled") << ")" << std::endl;
        
        while (running) {
            handleInput();
            runFrame();
        }
    }
    
    void cleanup() {
        if (inputFd >= 0) {
            close(inputFd);
            inputFd = -1;
        }
        
        if (audioDevice) {
            SDL_CloseAudioDevice(audioDevice);
            audioDevice = 0;
        }
        
        if (screenTexture) {
            SDL_DestroyTexture(screenTexture);
            screenTexture = nullptr;
        }
        
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        
        SDL_Quit();
    }
    
    // Platform callback - dummy implementation
    void platform_write_log(Platform::LogLevel level, const char* msg) {
        switch (level) {
            case Platform::LogLevel::Debug:   std::cout << "[DEBUG] "; break;
            case Platform::LogLevel::Info:    std::cout << "[INFO] "; break;
            case Platform::LogLevel::Warn:    std::cout << "[WARN] "; break;
            case Platform::LogLevel::Error:   std::cout << "[ERROR] "; break;
        }
        std::cout << msg << std::endl;
    }
};

// Signal handler for clean exit
MinimalFrontend* g_frontend = nullptr;
void signal_handler(int signal) {
    if (g_frontend) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        // Set running to false - the main loop will exit cleanly
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    bool debugMode = false;
    std::string romPath;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [--debug] <rom_file>" << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << argv[0] << " /path/to/game.nds" << std::endl;
        std::cerr << "  " << argv[0] << " --debug /path/to/game.nds  (shows button codes)" << std::endl;
        return 1;
    }
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            debugMode = true;
        } else {
            romPath = arg;
        }
    }
    
    if (romPath.empty()) {
        std::cerr << "Error: No ROM file specified" << std::endl;
        return 1;
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize frontend
    MinimalFrontend frontend;
    g_frontend = &frontend;
    
    if (!frontend.initialize()) {
        std::cerr << "Failed to initialize frontend" << std::endl;
        return 1;
    }
    
    if (debugMode) {
        std::cout << std::endl;
        std::cout << "=== DEBUG MODE: Button Mapping Detection ===" << std::endl;
        std::cout << "Press each button on your handheld to see its code." << std::endl;
        std::cout << "This will help you customize the button mappings if needed." << std::endl;
        std::cout << "Press ESC or Ctrl+C to continue to game loading..." << std::endl;
        std::cout << std::endl;
        
        // Run input debug loop
        frontend.debugInputMapping();
    }
    
    if (!frontend.loadROM(romPath)) {
        std::cerr << "Failed to load ROM: " << romPath << std::endl;
        return 1;
    }
    
    // Run the emulation
    frontend.run();
    
    std::cout << "melonDS exiting..." << std::endl;
    return 0;
} 