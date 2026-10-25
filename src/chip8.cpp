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
    // Only the first instruction after a 60 Hz tick may draw (original display-wait behavior)
    vblankNow = vblankPending;
    vblankPending = false;

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
        case 0x2: OP_2nnn(); break;
        case 0x3: OP_3xkk(); break;
        case 0x4: OP_4xkk(); break;
        case 0x5: OP_5xy0(); break;
        case 0x6: OP_6xkk(); break;
        case 0x7: OP_7xkk(); break;
        case 0x8:
            switch (opcode & 0x000F) {
                case 0x0: OP_8xy0(); break;
                case 0x1: OP_8xy1(); break;
                case 0x2: OP_8xy2(); break;
                case 0x3: OP_8xy3(); break;
                case 0x4: OP_8xy4(); break;
                case 0x5: OP_8xy5(); break;
                case 0x6: OP_8xy6(); break;
                case 0x7: OP_8xy7(); break;
                case 0xE: OP_8xyE(); break;
                default:
                    std::cerr << "Unhandled opcode: " << std::hex << opcode << "\n";
                    break;
            }
            break;
        case 0x9: OP_9xy0(); break;
        case 0xA: OP_Annn(); break;
        case 0xB: OP_Bnnn(); break;
        case 0xC: OP_Cxkk(); break;
        case 0xD: OP_Dxyn(); break;
        case 0xE: 
            switch (opcode & 0x00FF) {
                case 0x9E: OP_Ex9E(); break;
                case 0xA1: OP_ExA1(); break;
                default:
                    std::cerr << "Unhandled opcode: " << std::hex << opcode <<"\n";
                    break;
            }
            break;
        case 0xF:
            switch( opcode & 0x00FF) {
                case 0x07: OP_Fx07(); break;
                case 0x0A: OP_Fx0A(); break;
                case 0x15: OP_Fx15(); break;
                case 0x18: OP_Fx18(); break;
                case 0x1E: OP_Fx1E(); break;
                case 0x29: OP_Fx29(); break;
                case 0x33: OP_Fx33(); break;
                case 0x55: OP_Fx55(); break;
                case 0x65: OP_Fx65(); break;
                default:
                    std::cerr << "Unhandled opcode: " << std::hex << opcode << "\n";
                    break;
            }
            break;
        default:
            std::cerr << "Unhandled opcode: " << std::hex << opcode << "\n";
            break;
    }
}

// Both timeres count down by 1 at 60 Hz while above zero. Called by main every 1/60s
void Chip8::UpdateTimers() {
    if (delayTimer > 0) --delayTimer;
    if (soundTimer >0)  --soundTimer;
    vblankPending = true;
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

// Save the return address on the stack, then jump. PC was already advanced by 2 in Cycle(), so it points at the instruction after the call
void Chip8::OP_2nnn() {
    uint16_t address = opcode & 0x0FFF;
    stack[sp] = pc;     // push the return address
    ++sp;               // move the stack pointer up to the next free slot
    pc = address;       // jump to the subroutine
}

// Skip the next instruction if Vx = kk. pc already points at the next instruction, so adding 2 more steps over it
void Chip8::OP_3xkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    if (registers[Vx] == byte) {
        pc += 2;
    }
}

// Skip the next instruction if Vx != kk
void Chip8::OP_4xkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    if (registers[Vx] != byte) {
        pc += 2;
    }
}

// Skip the next instruction if Vx == Vy
void Chip8::OP_5xy0() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    if (registers[Vx] == registers[Vy]) {
        pc += 2;
    }
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

// Set Vx = Vy
void Chip8::OP_8xy0() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    registers[Vx] = registers[Vy];
}

// Set Vx = Vx OR Vy
void Chip8::OP_8xy1() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    registers[Vx] |= registers[Vy];
    registers[0xF] = 0;
}

// Set Vx = Vx AND Vy
void Chip8::OP_8xy2() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    registers[Vx] &= registers[Vy];
    registers[0xF] = 0;
}

// Set Vx = Vx XOR Vy
void Chip8::OP_8xy3() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    registers[Vx] ^= registers[Vy];
    registers[0xF] = 0;
}

// Sum is computed in 16 bits so a result over 255 isn't lost, then only the lowest 8 bits are stored back in Vx.
// If the sum is greater than 255, VF is set to 1, otherwise 0.
// Flag is written last so that if x is F, the flag isn't overwritten by the result.
void Chip8::OP_8xy4() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint16_t sum = registers[Vx] + registers[Vy];
    registers[Vx] = sum & 0xFF;
    registers[0xF] = (sum > 255) ? 1 : 0;
}

// Set Vx = Vx - Vy. If Vx > Vy, then VF is set to 1, otherwise 0.
void Chip8::OP_8xy5() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint8_t flag = (registers[Vx] >= registers[Vy]) ? 1 : 0;
    registers[Vx] -= registers[Vy];
    registers[0xF] = flag;
}

// Set Vx = Vy shifted right by 1. VF = least significant bit of Vy that was shifted out.
void Chip8::OP_8xy6() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint8_t flag = registers[Vy] & 0x1;
    registers[Vx] = registers[Vy] >> 1;
    registers[0xF] = flag;
}

// Set Vx = Vy - Vx. If Vy > Vx, then VF is set to 1, otherwise 0.
void Chip8::OP_8xy7() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint8_t flag = (registers[Vy] >= registers[Vx]) ? 1 : 0;
    registers[Vx] = registers[Vy] - registers[Vx];
    registers[0xF] = flag;
}

// Set Vx = Vy shifted left by 1. VF = most significant bit of Vy that was shifted out.
void Chip8::OP_8xyE() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint8_t flag = (registers[Vy] & 0x80) >> 7;
    registers[Vx] = registers[Vy] << 1;
    registers[0xF] = flag;
}

// Skip the next instruction if Vx != Vy.
void Chip8::OP_9xy0() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    if (registers[Vx] != registers[Vy]) {
        pc += 2;
    }
}

// Set the index register to nnn: where the sprite data lives for drawing
void Chip8::OP_Annn() {
    index = opcode & 0x0FFF;
}

// Jump to address nnn + V0. Keep the last 3 hex digits, add the value in V0, and load the result into pc
void Chip8::OP_Bnnn() {
    uint16_t address = opcode & 0x0FFF;
    pc = address + registers[0];
}

// Set Vx = random byte AND kk. AND mask is used to limit which bits of the random number are kept
void Chip8::OP_Cxkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    registers[Vx] = randByte(randGen) & byte;
}

// Draw an n-byte sprite from memory[index] at screen position (Vx, Vy).
// Each byte = one row of 8 pixels. Pixels are XORed onto the screen and VF is set to 1 if a collision occurs (a lit pixel gets turned off)
void Chip8::OP_Dxyn() {
    // If this frame's draw is already used, undo the pc advance so this same instruction retries on the next cycle
    if (displayWait && !vblankNow) {
    pc -= 2;
    return;
    }

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
            if (xPos + col >= VIDEO_WIDTH) break;                // clip at right edge

            // 0x80 >> col is a single set bit moving from the leftmost pixel to the rightmost
            if (spriteByte & (0x80 >> col)) {
                uint32_t& pixel = video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];
                if (pixel == 0xFFFFFFFF) registers[0xF] = 1;     // erasing a lit pixel = collision
                pixel ^= 0xFFFFFFFF;                             // XOR toggles the pixel
            }
        }
    }
}

// Skip next instruction if key whose number in Vx is pressed. Masked with 0x0F so a value above 15 can't index outside the keypad array.
void Chip8::OP_Ex9E() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t key = registers[Vx] & 0x0F;
    if (keypad[key]) {
        pc += 2;
    }
}

// Skip the next instruction if the key whose number is in Vx is NOT pressed
void Chip8::OP_ExA1() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t key = registers[Vx] & 0x0F;
    if (!keypad[key]) {
        pc += 2;
    }   
}

// Set Vx = delay timer value
void Chip8::OP_Fx07() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    registers[Vx] = delayTimer;
}

// Wait for a key press and release, then store the key in Vx. While waiting move pc back 2 so this same isntruction repeats each cycle. The rest of the emulator keeps running.
void Chip8::OP_Fx0A() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    if (!waitingForRelease) {
        // Phase 1: wait for any key to go down
        for (uint8_t i = 0; i < 16; ++i) {
            if (keypad[i]) {
                waitKey = i;
                waitingForRelease = true;
                break;
            }
        }
        pc -= 2;            // repeat this instruction next cycle
    }

    else if (keypad[waitKey]) {
        // Phase 2: the key is still held down, so keep waiting
        pc -= 2;
    }

    else {
        // Phase 3: the key was released, so store it and move on
        registers[Vx] = waitKey;
        waitingForRelease = false;
    }
}

// Set delay timer = Vx
void Chip8::OP_Fx15() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    delayTimer = registers[Vx];
}

// Set sound timer = Vx
void Chip8::OP_Fx18() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    soundTimer = registers[Vx];
}

// Set I = I + Vx
void Chip8::OP_Fx1E() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    index += registers[Vx];
}

// Set I = address of the font sprite for the digit in Vx. Each font char is 5 bytes, so 5(digit) gives the offset from the start of the font data
void Chip8::OP_Fx29() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t digit = registers[Vx] & 0x0F;
    index = FONTSET_START_ADDRESS + (5 * digit);
}

// Store the decimal digits of Vx in memory at I (hundreds), I+1 (tens), I+2 (ones)
void Chip8::OP_Fx33() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t value = registers[Vx];

    memory[index + 2] = value % 10;     // ones
    value /= 10;
    memory[index + 1] = value % 10;     // tens
    value /= 10;
    memory[index] = value % 10;         // hundreds
}

// Store registers V0 through Vx in memory starting at I
void Chip8::OP_Fx55() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    for (uint8_t i = 0; i <= Vx; ++i) {
        memory[index + i] = registers[i];
    }
    index += Vx + 1;
}

// Read registers V0 through Vx from memory starting at I
void Chip8::OP_Fx65() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    for (uint8_t i = 0; i <= Vx; ++i) {
        registers[i] = memory[index + i];
    }
    index += Vx + 1;
}