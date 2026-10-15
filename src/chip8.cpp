#include "chip8.hpp"
#include <cstring>
#include <fstream>          // std::ifstream for reading files
#include <iostream>         // std::cerr for error messages

constexpr unsigned int START_ADDRESS = 0x200;           // ROMs are loaded starting at 0x200 by initial design
constexpr unsigned int  FONTSET_START_ADDRESS = 0x50;   // Built-in font sprites location
constexpr unsigned int FONTSET_SIZE = 80;

// Array of font bytes. 16 characters represented by 5 bytes each.
// Each byte is one row of the character, 8 pixels wide. a 1 bit is a lit pixel.
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

// Chip8 function belonging to constructor Chip8 class
Chip8::Chip8() {
    // First instruction of every program is at 0x200
    pc = START_ADDRESS;
    // Copies font data into memory at 0x50 one byte at a time
    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
}

void Chip8::LoadROM(const char* filename) {
    // Open file in bianry mode. "ate" starts the read cursor at the end to laern the file's size
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
}

void Chip8::OP_00E0() {
    std::memset(video, 0, sizeof(video));
}