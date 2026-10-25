#pragma once            // Stops header from being included twice in one file       
#include <cstdint>      // Fixed size types 
#include <random>       // Random number generator for Cxkk

constexpr unsigned int VIDEO_WIDTH = 64;
constexpr unsigned int VIDEO_HEIGHT = 32;

class Chip8 {
public:
    Chip8();                                        // Sets up the machine's starting state
    bool LoadROM(const char* filename);             // Copies ROM file into memory; false on failure
    void Fault(const char* message);                // Report a fatal emulation error and stop executing
    bool InMemory(unsigned int start, unsigned int count) const;  // True if count bytes from start fit in memory
    void Cycle();                                   // Runs one fetch, decode, execute step
    void UpdateTimers();                            // Ticks both timers down by 1

    // Opcode Declarations
    void OP_00E0();     // CLS
    void OP_00EE();     // RET
    void OP_1nnn();     // JP addr
    void OP_2nnn();     // CALL addr
    void OP_3xkk();     // SE Vx, byte
    void OP_4xkk();     // SNE Vx, byte
    void OP_5xy0();     // SE Vx, Vy
    void OP_6xkk();     // LD Vx, byte
    void OP_7xkk();     // ADD Vx, byte
    void OP_8xy0();     // LD Vx, Vy                               
    void OP_8xy1();     // OR Vx, Vy
    void OP_8xy2();     // AND Vx, Vy
    void OP_8xy3();     // XOR Vx, Vy
    void OP_8xy4();     // ADD Vx, Vy
    void OP_8xy5();     // SUB Vx, Vy
    void OP_8xy6();     // SHR Vx                       
    void OP_8xy7();     // SUBN Vx, Vy
    void OP_8xyE();     // SHL Vx {, Vy}
    void OP_9xy0();     // SNE Vx, Vy
    void OP_Annn();     // LD I, addr
    void OP_Bnnn();     // JP V0, addr
    void OP_Cxkk();     // RND Vx, byte
    void OP_Dxyn();     // DRW Vx, Vy, nibble
    void OP_Ex9E();     // SKP Vx
    void OP_ExA1();     // SKNP Vx
    void OP_Fx07();     // LD Vx, DT
    void OP_Fx0A();     // LD Vx, K
    void OP_Fx15();     // LD DT, Vx
    void OP_Fx18();     // LD ST, Vx
    void OP_Fx1E();     // ADD I, Vx
    void OP_Fx29();     // LD F, Vx
    void OP_Fx33();     // LD B, Vx
    void OP_Fx55();     // LD [I], Vx
    void OP_Fx65();     // LD Vx, [I]

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

    std::mt19937 randGen{std::random_device{}()};           // Random number generator, seeded from the OS
    std::uniform_int_distribution<int> randByte{0, 255};    // Picks a whole number from 0 to 255 evenly
    bool vblankPending{false};                              // Set by UpdateTimers() at each 60 Hz tick
    bool vblankNow{false};                                  // True only for the first instruction executed after a tick
    bool waitingForRelease{false};                          // Fx0A: a key has been pressed and we are waiting for it to be released
    uint8_t waitKey{0};                                     // Fx0A: which key we are waiting on
    bool displayWait{true};                                 // true = original behavior (one draw per frame); toggle at runtime
    bool halted{false};                                     // Set by Fault(); Cycle() does nothing once true
};