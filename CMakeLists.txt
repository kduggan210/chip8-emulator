cmake_minimum_required(VERSION 3.21)    
project(chip8 CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
add_executable(chip8 src/main.cpp src/chip8.cpp)
target_compile_options(chip8 PRIVATE -Wall -Wextra)     
target_link_libraries(chip8 PRIVATE SDL2::SDL2main SDL2::SDL2)

if (WIN32)
    add_custom_command(TARGET chip8 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:chip8> $<TARGET_FILE_DIR:chip8>
        COMMAND_EXPAND_LISTS)
endif()