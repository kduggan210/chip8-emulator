#include <SDL.h>
#include <iostream>
#include "chip8.hpp"

int main(int argc, char* argv[]) {
    // Require a ROM path so a missing argument gives a message instead of a crash
    if (argc < 2) {
        std::cerr << "Usage: chip8 <ROM file>\n";
        return 1;
    }

    // Create the machine and load the ROM into memory at 0x200
    Chip8 chip8;               
    chip8.LoadROM(argv[1]);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
        }

    // Each CHIP-8 pixel becomes a scale x scale block so the window is 640x320
    const int scale = 10;
    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        VIDEO_WIDTH * scale, VIDEO_HEIGHT * scale, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Image held by the renderer. 64x32 (one entry per CHIP-8 pixel). "Streaming" means contents are overwritten every frame
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        VIDEO_WIDTH, VIDEO_HEIGHT);

    bool running = true;
    SDL_Event event;
    while (running) {
        // Closing the window ends the loop
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // Run one fetch-decode-execute step
        chip8.Cycle();

        // Copy the video buffer into the texture. Last argument tells SDL where each row of the flat array begins (Pitch: bytes per row)
        SDL_UpdateTexture(texture, nullptr, chip8.video, VIDEO_WIDTH * sizeof(uint32_t));

        // Draw the texture stretched to fill the whole window
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        // Pause between cycles so emulator doesn't run at full CPU speed
        SDL_Delay(2);
    }

    // Clean up in reverse order of creation
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}