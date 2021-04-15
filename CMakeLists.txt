cmake_minimum_required(VERSION 3.20)
project(chip8)

set(CMAKE_CXX_STANDARD 11)
set(SDL_DIR /usr/local/Cellar/sdl2/2.0.14_1)
include_directories(${SDL_DIR}/include)
link_directories(${SDL_DIR}/lib)

add_executable(
        ${PROJECT_NAME} 
        src/main.cpp
        src/chip8.cpp
        src/platform.cpp)

target_link_libraries(${PROJECT_NAME} SDL2)