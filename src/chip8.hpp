#pragma once            // Stops header from being included twice in one file       
#include <cstdint>      // Fixed size types 

constexpr unsigned int VIDEO_WIDTH = 64;
constexpr unsigned int VIDEO_HEIGHT = 32;

class Chip8 {
public:
    Chip8();                                        // Sets up the machine's starting state
    void LoadROM(const char* filename);             // Copies ROM file into memory
    void Cycle();                                   // Runs one fetch, decode, execute step

    uint8_t registers[16]{};                        // 16 8-bit registers V0-VF
    uint8_t memory[4096]{};                         // 4 KB of RAM 0x000-0xFFF
    uint16_t index{};                               // Holds memory addresses
    uint16_t pc{};                                  // Holds address of next instruction
    uint16_t stack[16]{};                           // Return addresses for subroutine calls
    uint8_t sp{};                                   // Stack pointer pointing to stack slot at the top
    uint8_t delayTimer{};                           // Counts down to 0 for game timing
    uint8_t soundTimer{};                           // Buzzer is on while value is non-zero
    uint8_t keypad[16]{};                           // State of each key; 1 = pressed, 0 = not pressed
    uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT]{};   // One on or off value per pixel
    uint16_t opcode{};                              // Current instruction being executed
};