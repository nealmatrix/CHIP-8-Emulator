# CHIP-8 Emulator

## Usage
After download or clone the repository, I use CMake to build the code, then run the `chip8` program.

The default game is Tetris. If you want to use CLI, please uncomment `#define CLI` in the main.cpp file.

### Command Line Usage
chip8 \<Scale> \<Delay> \<Rom>

Scale: scale factor to enlarge the game window, 10 is recommanded.

Delay: time between the cycles to determine the game speed.

Rom: CHIP-8 file to load

## Prerequisite
1. CMake  
    For MacOS, you can download CMake using `brew install CMake`.  
    If you use M1 chip Mac, please open terminal using Rosetta.

2. SDL2 library  
    For MacOS, you can download SDL2 using `brew install sdl2`.  
    If you use M1 chip Mac, please open terminal using Rosetta.  
    You can use `brew info sdl2` to find the directory of SDL2 library.

## Note
Please change the SDL2 library directory ${SDL_DIR} to your own directory in the CMakeLists.txt file.

## Acknowledge
Special thanks to Austin Morlan and his detailed guide of CHIP-8 emulator.