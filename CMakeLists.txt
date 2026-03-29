cmake_minimum_required(VERSION 3.22)
project(ability_hand_test C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Submodules
add_subdirectory(external/byte-stuffing)
add_subdirectory(external/serial-cross-platform)
add_subdirectory(external/dartt-protocol)


# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Create the wav_parser executable
add_executable(wav_parser
    src/main.cpp
    src/args.cpp
    src/wav_parsing.cpp
	embedded/audio.c
	src/AudioWriter.cpp
)

target_include_directories(wav_parser PRIVATE src embedded)

target_link_libraries(wav_parser
    PRIVATE
    serial
	dartt_protocol
    dartt_checksum
    cobs
    PPP
)


# Set output directory
set_target_properties(wav_parser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)