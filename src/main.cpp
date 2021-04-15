#include "chip8.hpp"
#include "platform.hpp"
#include <iostream>
#include <chrono>

int main(int argc, char *argv[]){
    int videoScale;
    int cycleDelay;
    char const *romFilename;
    
//#define CLI
# ifdef CLI
    if (argc != 4){
        std::cerr << "Usage:" << argv[0] << " <Scale> <Delay> <ROM> \n";
        std::exit(EXIT_FAILURE);
    }

    videoScale = std::stoi(argv[1]);
    cycleDelay = std::stoi(argv[2]);
    romFilename = argv[3];
# else
    videoScale = 10;
    cycleDelay = 1;
    char const *romFilenameAll[] = {
        "..//roms//test//BC_test.ch8",
        "..//roms//test//test_opcode.ch8",
        "..//roms//game//TETRIS"};

    romFilename = romFilenameAll[2]; 
# endif

    Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);
    
    Chip8 chip8;
    chip8.LoadROM(romFilename);

    int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

    auto lastCycleTime = std::chrono::high_resolution_clock::now();
    bool quit = false;

    while(!quit){
        quit = platform.ProcessInput(chip8.keypad);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

        if (dt > cycleDelay){
            lastCycleTime = currentTime;

            chip8.Cycle();

            platform.Update(chip8.video, videoPitch);
        }
    }
    return 0;
}