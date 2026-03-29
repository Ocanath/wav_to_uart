cmake_minimum_required(VERSION 3.22)
project(dartt_audio C CXX)

option(DARTT_AUDIO_BUILD_EXAMPLES "Build dartt_audio example programs" OFF)

# Only apply global compiler flags when built as the top-level project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
endif()

# Dependencies: use parent project targets if already defined, otherwise pull from external/.
# This allows a parent CMake project to supply dartt-protocol, serial-cross-platform, and
# byte-stuffing rather than building duplicate copies.
if(NOT TARGET dartt_protocol)
    add_subdirectory(external/dartt-protocol)
endif()
if(NOT TARGET serial)
    add_subdirectory(external/serial-cross-platform)
endif()
if(NOT TARGET cobs)
    add_subdirectory(external/byte-stuffing)
endif()

# Library
add_library(dartt_audio STATIC
    src/AudioWriter.cpp
    src/wav_parsing.cpp
)

target_compile_features(dartt_audio PUBLIC cxx_std_20)

# src/ exposes AudioWriter.h, wav_parsing.h, dr_wav.h
# embedded/ exposes audio.h (shared with embedded firmware)
target_include_directories(dartt_audio
    PUBLIC
        src
        embedded
)

target_link_libraries(dartt_audio
    PUBLIC
        dartt_protocol
        dartt_checksum
        serial
        cobs
)

# Examples — only built when DARTT_AUDIO_BUILD_EXAMPLES=ON
if(DARTT_AUDIO_BUILD_EXAMPLES)
    add_executable(example_playback
        examples/example_playback.cpp
        examples/args.cpp
    )

    target_include_directories(example_playback PRIVATE examples)

    target_link_libraries(example_playback PRIVATE dartt_audio)

    set_target_properties(example_playback PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
endif()
