cmake_minimum_required(VERSION 3.10)
project(CHIP8CHAPA)
set(CMAKE_CXX_STANDARD 17)

# SDL2
set(SDL2_DIR "${CMAKE_SOURCE_DIR}/deps/SDL2-2.32.8")
set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include")
set(SDL2_LIB_DIR "${SDL2_DIR}/lib")
include_directories(${SDL2_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})

add_library(chip8_memory chip8_memory.cpp)
add_library(chip8_registers chip8_registers.cpp)
add_library(chip8_timers chip8_timers.cpp)
add_library(chip8_input chip8_input.cpp)
add_library(chip8_display chip8_display.cpp)
add_library(chip8_sound chip8_sound.cpp)
add_library(chip8_cpu chip8_cpu.cpp)

add_executable(chip8chapa main.cpp config.cpp)
target_link_libraries(chip8chapa chip8_cpu chip8_memory chip8_registers chip8_timers chip8_input chip8_display chip8_sound SDL2main SDL2) 

# Set output executable name to CHIP8CHAPA (all caps) on Windows
if (WIN32)
    set_target_properties(chip8chapa PROPERTIES
        WIN32_EXECUTABLE YES
        OUTPUT_NAME "CHIP8CHAPA"
    )
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/chip8chapa.rc")
    target_sources(chip8chapa PRIVATE ${APP_ICON_RESOURCE_WINDOWS})
endif() 