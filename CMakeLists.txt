cmake_minimum_required(VERSION 3.10)
project(wav_to_rs485 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Create the wav_parser executable
add_executable(wav_parser wav_parser.c)

# Make sure dr_wav.h is available
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dr_wav.h")
    message(FATAL_ERROR "dr_wav.h not found in project directory")
endif()

# Set output directory
set_target_properties(wav_parser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)