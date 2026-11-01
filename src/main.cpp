#include <SDL.h>
#include <iostream>
#include <string>
#include <atomic>
#include "chip8.hpp"

// Maps a keyboard key to a CHIP-8 keypad index or -1 if unmapped.
// CHIP-8:  1 2 3 C        Keyboard:  1 2 3 4
//          4 5 6 D                   Q W E R
//          7 8 9 E                   A S D F
//          A 0 B F                   Z X C V
int MapKey(SDL_Keycode key) {
    switch (key) {
        case SDLK_x: return 0x0;
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_q: return 0x4;
        case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;
        case SDLK_a: return 0x7;
        case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;
        case SDLK_z: return 0xA;
        case SDLK_c: return 0xB;
        case SDLK_4: return 0xC;
        case SDLK_r: return 0xD;
        case SDLK_f: return 0xE;
        case SDLK_v: return 0xF;
        default:     return -1;
    }
}

// Parse a whole-number argument that must fall in [lo, hi]. Returns false if it doesn't.
bool ParseInt(const char* text, int lo, int hi, int& out) {
    try {
        std::size_t used = 0;
        int value = std::stoi(text, &used);
        if (used != std::string(text).size() || value < lo || value > hi) return false;
        out = value;
        return true;
    } catch (...) {             // stoi throws on non-numeric or out-of-range text
        return false;
    }
}

// State shred with the audio thread. The main thread sets 'on'; SDL's audio thread reads it. SDL calls the callback from a separate thread, which is why 'on' is atomic.
struct Beeper {
    std::atomic<bool> on{false};
    double phase{0.0};          // position within one wave cycle (0 to 1); only the audio thread uses it 
};

// SDL calls this whenever the audio device needs more samples. It fills them with a 440 Hz square wave while the beeper is on, and with silence otherwise.
void AudioCallback(void* userdata, Uint8* stream, int len) {
    Beeper* beeper = static_cast<Beeper*>(userdata);
    Sint16* samples = reinterpret_cast<Sint16*>(stream);        // the buffer holds 16-bit samples
    int count = len / static_cast<int>(sizeof(Sint16));         // len is in bytes, so convert to samples
    const double step = 440.0 / 44100.0;                        // how far the wave advances per sample

    for (int i = 0; i < count; ++i) {
        if (beeper->on.load()) {
            samples[i] = (beeper->phase < 0.5) ? 2000 : -2000;   // high for half the cycle, low for the other half
            beeper->phase += step;
            if (beeper->phase >= 1.0) beeper->phase -= 1.0;
        } else {
            samples[i] = 0;                                       // silence
            beeper->phase = 0.0;
        }
    }
}

int main(int argc, char* argv[]) {
    // Require a ROM path so a missing argument gives a message instead of a crash
    if (argc < 2) {
    std::cerr << "Usage: chip8 <ROM file> [cycles per frame] [scale]\n";
    return 1;
    }

    int cyclesPerFrame = 12;    // default: about 720 instructions per second
    int scale = 10;             // default: 640x320 window
    if (argc >= 3 && !ParseInt(argv[2], 1, 10000, cyclesPerFrame)) {
        std::cerr << "Cycles per frame must be a whole number from 1 to 10000\n";
        return 1;
    }
    if (argc >= 4 && !ParseInt(argv[3], 1, 40, scale)) {
        std::cerr << "Scale must be a whole number from 1 to 40\n";
        return 1;
    }

    // Create the machine and load the ROM into memory at 0x200
    Chip8 chip8;               
    if (!chip8.LoadROM(argv[1])) return 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // Each CHIP-8 pixel becomes a scale x scale block. 640x320 at the default scale of 10.
    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        VIDEO_WIDTH * scale, VIDEO_HEIGHT * scale, SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Could not create window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        std::cerr << "Could not create renderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Image held by the renderer. 64x32 (one entry per CHIP-8 pixel). "Streaming" means contents are overwritten every frame
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        VIDEO_WIDTH, VIDEO_HEIGHT);

    if (!texture) {
        std::cerr << "Could not create texture: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Audio is optional: if it can't start, the emulator keeps running without sound
    Beeper beeper;
    SDL_AudioDeviceID audioDevice = 0;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
        SDL_AudioSpec want{};
        want.freq = 44100;            // samples per second
        want.format = AUDIO_S16SYS;   // signed 16-bit samples
        want.channels = 1;            // mono
        want.samples = 512;           // buffer size in samples
        want.callback = AudioCallback; 
        want.userdata = &beeper;      // passed to the callback as 'userdata'
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
    }

    if (audioDevice != 0) {
        SDL_PauseAudioDevice(audioDevice, 0);       // 0 = start playing (silence until 'on' is set)
    }   else {
        std::cerr << "Audio unavailable, continuing without sound: " << SDL_GetError() << "\n";
    }

    // Run a batch of instructions, tick the timers once, and redraw once every 1/60 s
    const double frameMs = 1000.0 / 60.0;
    const double ticksPerMs = SDL_GetPerformanceFrequency() / 1000.0;
    Uint64 lastCounter = SDL_GetPerformanceCounter();
    double accumulator = 0.0;           // Time not yet used on frames

    bool running = true;
    bool releasePending[16]{};          // keys released but not yet shown to the ROM
    SDL_Event event;
    while (running) {
        // Closing the window ends the loop, quit key, and keypad presses and releases
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT){
                running = false;
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (event.key.keysym.sym == SDLK_F1) {
                    chip8.displayWait = !chip8.displayWait;
                    std::cout << "Display wait: " << (chip8.displayWait ? "on" : "off") << "\n";
                }
                int key = MapKey(event.key.keysym.sym);
                if (key != -1) {
                    chip8.keypad[key] = 1;
                    releasePending[key] = false;        // a new press cancels a pending release
                }
            }
            else if (event.type == SDL_KEYUP) {
                int key = MapKey(event.key.keysym.sym);
                if (key != -1) releasePending[key] = true;  // don't clear yet
            }
        }

        // Add the real time since the last loop pass to the accumulator
        Uint64 now = SDL_GetPerformanceCounter();
        accumulator += (now - lastCounter) / ticksPerMs;
        lastCounter = now;
        if (accumulator > 100.0) accumulator = frameMs;

        // Spend accumulated time in 1/60 s frames
        bool redraw = false;
        while (accumulator >= frameMs) {
            accumulator -= frameMs;
            for (int i = 0; i < cyclesPerFrame; ++i) chip8.Cycle();
            chip8.UpdateTimers();
            redraw = true;
        }

        // The buzzer shounds while the sound timer is above zero
        beeper.on = (chip8.soundTimer > 0);

        // Only update when a frame has run
        if (redraw) {
            // Apply release only after a frame has run, so every press is seen at least once
            for (int i = 0; i < 16; ++i) {
                if (releasePending[i]) {
                    chip8.keypad[i] = 0;
                    releasePending[i] = false;
                }
            }
            
            // Draw the latest video buffer
            SDL_UpdateTexture(texture, nullptr, chip8.video, VIDEO_WIDTH * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }

        SDL_Delay(1);
    }

    // Clean up in reverse order of creation
    if (audioDevice != 0) SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}