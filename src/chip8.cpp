#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>
#include "chip8.hpp"

const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;

uint8_t fontset[FONTSET_SIZE] = {
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

Chip8::Chip8()
    :randGen(std::chrono::system_clock::now().time_since_epoch().count()){
    //Initialize PC
    pc = START_ADDRESS;

    //Load fonts into memory
    for (unsigned int i = 0; i < FONTSET_SIZE; ++i){
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }

    //Initialize RNG
    randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

    //Set up function pointer table
    table[0x0] = &Chip8::Table0;
    table[0x1] = &Chip8::OP_1nnn;
    table[0x2] = &Chip8::OP_2nnn;
    table[0x3] = &Chip8::OP_3xkk;
    table[0x4] = &Chip8::OP_4xkk;
    table[0x5] = &Chip8::OP_5xy0;
    table[0x6] = &Chip8::OP_6xkk;
    table[0x7] = &Chip8::OP_7xkk;
    table[0x8] = &Chip8::Table8;
    table[0x9] = &Chip8::OP_9xy0;
    table[0xA] = &Chip8::OP_Annn;
    table[0xB] = &Chip8::OP_Bnnn;
    table[0xC] = &Chip8::OP_Cxkk;
    table[0xD] = &Chip8::OP_Dxyn;
    table[0xE] = &Chip8::TableE;
    table[0xF] = &Chip8::TableF;

    table0[0x0] = &Chip8::OP_00E0;
    table0[0xE] = &Chip8::OP_00EE;

    table8[0x0] = &Chip8::OP_8xy0;
    table8[0x1] = &Chip8::OP_8xy1;
    table8[0x2] = &Chip8::OP_8xy2;
    table8[0x3] = &Chip8::OP_8xy3;
    table8[0x4] = &Chip8::OP_8xy4;
    table8[0x5] = &Chip8::OP_8xy5;
    table8[0x6] = &Chip8::OP_8xy6;
    table8[0x7] = &Chip8::OP_8xy7;
    table8[0xE] = &Chip8::OP_8xyE;

    tableE[0x1] = &Chip8::OP_ExA1;
    tableE[0xE] = &Chip8::OP_Ex9E;

    tableF[0x07] = &Chip8::OP_Fx07;
    tableF[0x0A] = &Chip8::OP_Fx0A;
    tableF[0x15] = &Chip8::OP_Fx15;
    tableF[0x18] = &Chip8::OP_Fx18;
    tableF[0x1E] = &Chip8::OP_Fx1E;
    tableF[0x29] = &Chip8::OP_Fx29;
    tableF[0x33] = &Chip8::OP_Fx33;
    tableF[0x55] = &Chip8::OP_Fx55;
    tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::LoadROM(char const* filename){
    //Open the file as a stream of binary and move the file pointer to the end
    std::ifstream file (filename, std::ios::binary | std::ios::ate);

    if (file.is_open()){
        //Get size of file and allocate a buffer to hold the contents
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        //Go back to the beginning of the file and fill the buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        //Load the ROM contents into the Chip8's memory, starting at 0x200
        for (long i = 0; i < size; ++i){
            memory[START_ADDRESS + i] = buffer[i];
        }

        //Free the buffer
        delete[] buffer;
    }
}

void Chip8::Cycle(){
    //Fetch
    opcode = (memory[pc] << 8u) | memory[pc + 1];

    //Increase PC before execute anything
    pc += 2;

    //Decode and Execute
    ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

    //Decrease the delay timer if it's been set
    if (delayTimer > 0)
        --delayTimer;
    
    //Decrease the sound timer if it's been set
    if (soundTimer > 0)
        --soundTimer;
}

void Chip8::OP_NULL(){

}
//00E0 - CLS: Clear the display
void Chip8::OP_00E0(){
    memset(video, 0, sizeof(video));
}
//00EE - RET: Return from a subroutine
void Chip8::OP_00EE(){
    --sp;
    pc = stack[sp];
}
//1nnn - JP addr: Jump to location nnn
//The interpreter sets the program counter to nnn
void Chip8::OP_1nnn(){
    uint16_t address =  opcode & 0x0FFFu;
    pc = address;
}
//2nnn - CALL addr: Call subroutine at nnn
void Chip8::OP_2nnn(){
    uint16_t address = opcode & 0x0FFFu;
    stack[sp] = pc;
    ++sp;
    pc = address;
}
//3xkk - SE Vx, byte: Skip next instruction if Vx = kk
void Chip8::OP_3xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] == byte){
        pc += 2;
    }
}
//4xkk - SNE Vx, byte: Skip next instruction if Vx != kk
void Chip8::OP_4xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] != byte){
        pc += 2;
    }
}
//5xy0 - SE Vx, Vy: Skip next instruction if Vx = Vy
void Chip8::OP_5xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy]){
        pc += 2;
    }
}
//6xkk - LD Vx, byte: Set Vx = kk
void Chip8::OP_6xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte; 
}
//7xkk - ADD Vx, byte: Set Vx = Vx + kk
void Chip8::OP_7xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFU;

    registers[Vx] += byte;
}
//8xy0 - LD Vx, Vy: Set Vx = Vy
void Chip8::OP_8xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}
//8xy1 - OR Vx, Vy: Set Vx = Vx OR Vy
void Chip8::OP_8xy1(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
}
//8xy2 - AND Vx, Vy: Set Vx = Vx AND Vy
void Chip8::OP_8xy2(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
}
//8xy3 - XOR Vx, Vy: Set Vx = Vx XOR Vy
void Chip8::OP_8xy3(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
}
//8xy4 - ADD Vx, Vy: Set Vx = Vx + Vy, set VF = carry 
void Chip8::OP_8xy4(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint16_t sum = registers[Vx] + registers[Vy];

    if (sum > 255u){
        registers[0xF] = 1;
    }else{
        registers[0xF] = 0;
    }

    registers[Vx] = sum & 0xFFu;
}
//8xy5 - SUB Vx, Vy: Set Vx = Vx - Vy, set VF = NOT borrow
void Chip8::OP_8xy5(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] >= registers[Vy]){
        registers[0xF] = 1;
    }else{
        registers[0xF] = 0;
    }

    registers[Vx] -= registers[Vy];
}
//8xy6 - SHR Vx: Set Vx = Vx SHR 1
//If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0
void Chip8::OP_8xy6(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = registers[Vx] & 0x1u;
    registers[Vx] >>= 1;
}
//8xy7 - SUBN Vx, Vy: Set Vx = Vy - Vx, set VF = NOT borrow
void Chip8::OP_8xy7(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vy] >= registers[Vx]){
        registers[0xF] = 1;
    }else{
        registers[0xF] = 0;
    }
    
    registers[Vx] = registers[Vy] - registers[Vx];
}
//8xyE - SHL Vx: Set Vx = Vx SHL 1
//If the most-significant bit of Vx is 1, then VF is set to 1, otherwise 0
void Chip8::OP_8xyE(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = (registers[Vx] & 0x80u) >> 7u;
    registers[Vx] <<= 1;
}
//9xy0 - SNE Vx, Vy: Skip next instruction if Vx != Vy
void Chip8::OP_9xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] != registers[Vy]){
        pc += 2;
    }
}
//Annn - LD I, addr: Set I = nnn
void Chip8::OP_Annn(){
    uint16_t address = opcode & 0x0FFFu;

    index = address;
}
//Bnnn - JP V0, addr: Jump to location nnn + V0
void Chip8::OP_Bnnn(){
    uint16_t address = opcode & 0x0FFFu;

    pc = registers[0x0] + address;
}
//Cxkk - RND Vx, byte: Set Vx = random byte AND kk
void Chip8::OP_Cxkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0xFFu;
    
    registers[Vx] = randByte(randGen) & byte;
}
//Dxyn - DRW Vx, Vy, nibble 
//Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
void Chip8::OP_Dxyn(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t height = opcode & 0x000Fu;

    uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
    uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

    registers[0xF] = 0;

    //spriteByte is 8-bit value stored in memory[I + row]
    //spritePixel is 1-bit value in spriteByte
    uint8_t spriteByte, spritePixel;

    uint32_t* screenPixelAddr; 
    for (unsigned int row = 0; row < height; ++row){
        spriteByte = memory[index + row];
        
        for (unsigned int col = 0; col < 8; ++ col){
            spritePixel = spriteByte & (0x80u >> col);
            screenPixelAddr = &video[(yPos + row) * VIDEO_WIDTH + xPos + col];

            if(spritePixel){
                if (!registers[0xF] && *screenPixelAddr){
                    registers[0xF] = 1;
                }

                *screenPixelAddr ^= 0xFFFFFFFFu;
            }    
        }
    }
}
//Ex9E - SKP Vx: Skip next instruction if key with the value of Vx is pressed
void Chip8::OP_Ex9E(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];

    if (keypad[key]){
        pc += 2;
    }
} 
//ExA1 - SKNP Vx: Skip next instruction if key with the value of Vx is not pressed
void Chip8::OP_ExA1(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];

    if (!keypad[key]){
        pc += 2;
    }
}
//Fx07 - LD Vx, DT: Set Vx = delay timer value
void Chip8::OP_Fx07(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[Vx] = delayTimer;
}
//Fx0A - LD Vx, K: Wait for a key press, store the value of the key in Vx
void Chip8::OP_Fx0A(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    unsigned int i = 0;
    for (; i < KEY_COUNT; ++i){
        if (keypad[i]){
            registers[Vx] = i;
            break;
        }
    }
    if (i == KEY_COUNT){
        pc -= 2;
    }
}
//Fx15 - LD DT, Vx: Set delay timer = Vx
void Chip8::OP_Fx15(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    delayTimer = registers[Vx];
}
//Fx18 - LD ST, Vx: Set sound timer = Vx
void Chip8::OP_Fx18(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    soundTimer = registers[Vx];
} 
//Fx1E - ADD I, Vx: Set I = I + Vx
void Chip8::OP_Fx1E(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index += registers[Vx];
}
//Fx29 - LD F, Vx: Set I = location of sprite for digit Vx
void Chip8::OP_Fx29(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index = FONTSET_START_ADDRESS + 5 * registers[Vx];
}
//Fx33 - LD B, Vx: Store BCD representation of Vx in memory locations I, I + 1, and I + 2
//The interpreter takes the decimal value of Vx, and places the hundreds digit in memory 
//at location in I, the tens digit at location I + 1, and the ones digit of location I + 2
void Chip8::OP_Fx33(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value  = registers[Vx];

    memory[index + 2] = value % 10;
    value /= 10;
    memory[index + 1] = value % 10;
    value /= 10;
    memory[index] = value % 10;
}
//Fx55 - LD [I], Vx: Store registers V0 through Vx in memory starting at location I
void Chip8::OP_Fx55(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i){
        memory[index + i] = registers[i]; 
    }
}
//Fx65 - LD Vx, [I]: Read registers V0 through Vx from memory starting at location I
void Chip8::OP_Fx65(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    
    for (uint8_t i = 0; i <= Vx; ++i){
        registers[i] = memory[index + i];
    }
}

//For function pointer table
void Chip8::Table0(){
    ((*this).*(table0[opcode & 0x000Fu]))();
}
void Chip8::Table8(){
    ((*this).*(table8[opcode & 0x000Fu]))();
}
void Chip8::TableE(){
    ((*this).*(tableE[opcode & 0x000Fu]))();
}
void Chip8::TableF(){
    ((*this).*(tableF[opcode & 0x00FFu]))();
}