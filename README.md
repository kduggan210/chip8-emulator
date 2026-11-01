# CHIP-8 Emulator

A CHIP-8 interpreter written in C++ with SDL2. It loads CHIP-8 programs (ROMs), runs them
through a fetch-decode-execute cycle, and shows the 64x32 display in a scalable window with
keyboard input and sound.

Built as a course project exploring virtual machines, interpretation, and low-level memory management.

## Features

- All 34 standard CHIP-8 instructions
- 60 Hz delay and sound timers, with a square-wave beep
- Configurable speed (instructions per frame) and window scale
- Clear errors for missing or oversized ROMs, and a safe halt when a ROM overflows the stack
  or accesses memory out of range

## Running the release build

Download the zip from the Releases page, unzip it, open PowerShell in the extracted folder, and run:

```
.\chip8.exe roms\games\pong.ch8
```

The required DLLs are included, so nothing needs to be installed.

## Usage

```
chip8 <ROM file> [cycles per frame] [scale]
```

| Argument | Default | Range | Meaning |
|---|---|---|---|
| ROM file | required | | Path to a `.ch8` file |
| cycles per frame | 12 | 1 to 10000 | Instructions per 1/60 s (12 is about 720 per second) |
| scale | 10 | 1 to 40 | Window pixels per CHIP-8 pixel (10 gives 640x320) |

To set the scale, give the cycles value too.
Examples (from a source build):

```
.\build\chip8.exe roms\games\pong.ch8
.\build\chip8.exe roms\games\tetris.ch8 12 15
.\build\chip8.exe roms\tests\6-keypad.ch8 50
```

## Controls

The CHIP-8 hex keypad is mapped onto the left side of the keyboard:

```
CHIP-8 keypad      Keyboard
1 2 3 C            1 2 3 4
4 5 6 D            Q W E R
7 8 9 E            A S D F
A 0 B F            Z X C V
```

| Key | Action |
|---|---|
| Esc | Quit |
| F1 | Toggle display wait (on by default) |

## Included ROMs

| ROM | Source | Cycles per frame | Notes |
|---|---|---|---|
| Pong | [dmatlack/chip8](https://github.com/dmatlack/chip8) | 12 | 1 and Q move the paddle up and down |
| Tetris | [dmatlack/chip8](https://github.com/dmatlack/chip8) | 12 | Q rotates, W moves left, E moves right |
| Br8kout | [chip8Archive](https://github.com/JohnEarnest/chip8Archive) | 7 | A and D move the paddle |
| Test suite (`roms/tests/`) | [Timendus/chip8-test-suite](https://github.com/Timendus/chip8-test-suite) (GPLv3) | 12 | Keypad test works best at 50 |

## Test results

| Test ROM | Result |
|---|---|
| IBM logo, CHIP-8 logo | Pass |
| Corax+ opcode test | All checks pass |
| Flags test | All checks pass |
| Keypad test | All three tests pass (at 50 cycles per frame) |
| Quirks test (CHIP-8 profile) | All 6 rows match |
| Beep test | Pass (holding C plays the tone; releasing it stops) |

## Known limitations

- Quirks are fixed to original CHIP-8 behavior, apart from the display wait toggle.
- SUPER-CHIP and XO-CHIP programs are not supported.
- The keypad test needs a higher speed (about 50 cycles per frame) to register rapid taps.

## Project structure

```
src/chip8.hpp     Chip8 class: machine state and instruction declarations
src/chip8.cpp     ROM loading, fetch-decode-execute, and all 34 instructions
src/main.cpp      SDL2 window, input, sound, timing loop, and argument parsing
roms/games/       Playable games
roms/tests/       Test suite ROMs and fault-handling test ROMs
CMakeLists.txt    Build configuration
```

## References

- Cowgod's Chip-8 Technical Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
- Austin Morlan, Building a CHIP-8 Emulator [C++]: https://austinmorlan.com/posts/chip8_emulator/
- Timendus, CHIP-8 test suite: https://github.com/Timendus/chip8-test-suite
- John Earnest, Octo: https://github.com/JohnEarnest/Octo
- SDL2 documentation: https://wiki.libsdl.org/SDL2/FrontPage