#include "chip8.hpp"
#include <cstring>          // std::memset for clearing the video buffer
#include <fstream>          // std::ifstream for reading files
#include <iostream>         // std::cerr for error messages

constexpr unsigned int START_ADDRESS = 0x200;           // ROMs are loaded starting at 0x200 by initial design
constexpr unsigned int  FONTSET_START_ADDRESS = 0x50;   // Built-in font sprites location
constexpr unsigned int FONTSET_SIZE = 80;

// Array of font bytes. 16 characters represented by 5 bytes each.
// Each byte is one row of the character, 8 pixels wide. A 1 bit is a lit pixel.
const uint8_t fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// Constructor: sets up the machine's starting state
Chip8::Chip8() {
    // First instruction of every program is at 0x200
    pc = START_ADDRESS;
    // Copies font data into memory at 0x50 one byte at a time
    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
}

void Chip8::LoadROM(const char* filename) {
    // Open file in binary mode. "ate" starts the read cursor at the end to learn the file's size
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Could not open ROM: " << filename << "\n";
        return; 
    }

    // Refuse ROMs that do not fit from 0x200 to 0xFFF memory space
    std::streamsize size = file.tellg();
    if (size > static_cast<std::streamsize>(sizeof(memory) - START_ADDRESS)) {
        std::cerr << "ROM is too large.\n";
        return;
    }

    // Move cursor back to start of the file
    file.seekg(0, std::ios::beg);
    // Read file into memory array converting uint8_t* into char* to match read() expectation
    file.read(reinterpret_cast<char*>(&memory[START_ADDRESS]), size);
}

void Chip8::Cycle() {
    // Fetch: shift byte at pc 8 bits and OR in byte at pc+1 providing the full 2 byte instruction 
    opcode = (memory[pc] << 8) | memory[pc+1];
    // Advance to next instruction before executing this one
    pc += 2;

    //Decode + Execute: the first hex digit picks the instruction family
    switch((opcode & 0xF000) >> 12) {
        case 0x0:
            if (opcode == 0x00E0) OP_00E0();
            else if (opcode == 0x00EE) OP_00EE();
            break;
        case 0x1: OP_1nnn(); break;
        case 0x6: OP_6xkk(); break;
        case 0x7: OP_7xkk(); break;
        case 0xA: OP_Annn(); break;
        case 0xD: OP_Dxyn(); break;
        default:
            std::cerr << "Unhandled opcode: " << std::hex << opcode << "\n";
            break;
    }
}


///////////////////////////////////////////////////////////////////////// OPCODE DEFINITIONS //////////////////////////////////////////////////////////////////////////


// Clear the display: Set every pixel to 0 (off)
void Chip8::OP_00E0() {
    std::memset(video, 0, sizeof(video));
}

// Return from a subroutine: pop the saved address off the stack into pc
void Chip8::OP_00EE() {
    --sp;
    pc = stack[sp];
}

// Jump to address nnn: keep the last three hex digits and load them into the program counter
void Chip8::OP_1nnn() {
    uint16_t address = opcode & 0x0FFF;
    pc = address;
}

// Set register Vx to kk. x = second hex digit, kk - last two hex digits
void Chip8::OP_6xkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    registers[Vx] = byte;
}

// Add kk to Vx. No carry flag so the 8-bit register wraps around past 255.
void Chip8::OP_7xkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    registers[Vx] += byte;
}

// Set the index register to nnn: where the sprite data lives for drawing
void Chip8::OP_Annn() {
    index = opcode & 0x0FFF;
}

// Draw an n-byte sprite from memory[index] at screen position (Vx, Vy).
// Each byte = one row of 8 pixels. Pixels are XORed onto the screen and VF is set to 1 if a collision occurs (a lit pixel gets turned off)
void Chip8::OP_Dxyn() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint8_t height = opcode & 0x000F;

    // The starting position wraps around the screen
    uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
    uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

    // Assume no collision until one is found 
    registers[0xF] = 0;

    for (unsigned int row = 0; row < height; ++row) {
        if (yPos + row >= VIDEO_HEIGHT) break;                  // clip at bottom edge
        uint8_t spriteByte = memory[index + row];

        for (unsigned int col = 0; col < 8; ++col) {
            if (xPos + col >= VIDEO_WIDTH) break;               // clip at right edge

            // 0x80 >> col is a single set bit moving from the leftmost pixel to the rightmost
            if (spriteByte & (0x80 >> col)) {
                uint32_t& pixel = video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];
                if (pixel == 0xFFFFFFFF) registers[0xF] = 1;     // erasing a lit pixel = collision
                pixel ^= 0xFFFFFFFF;                            // XOR toggles the pixel
            }
        }
    }
}