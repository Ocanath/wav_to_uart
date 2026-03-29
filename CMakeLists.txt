cmake_minimum_required(VERSION 3.22)
project(ability_hand_test C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Submodules
add_subdirectory(external/byte-stuffing)
add_subdirectory(external/serial-cross-platform)
add_subdirectory(external/dartt-protocol)

include(external/simpleble/cmake/prelude.cmake)
include(external/simpleble/cmake/epilogue.cmake)
add_subdirectory(external/simpleble/simpleble)

# v0.6.1 compiles with /WX; suppress MSVC STL array-iterator deprecation warnings
# that became errors in VS 2022 17.x+. Must be a compiler flag, not a source #define.
if(MSVC)
    target_compile_definitions(simpleble PRIVATE _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
endif()


# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Create the wav_parser executable
add_executable(wav_parser
    src/main.cpp
    src/args.cpp
    src/wav_parsing.cpp
    src/ble.cpp
	embedded/audio.c
	src/serial_callbacks.cpp
	src/AudioWriter.cpp
)

target_include_directories(wav_parser PRIVATE src embedded)

target_link_libraries(wav_parser
    PRIVATE
    simpleble::simpleble
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