cmake_minimum_required(VERSION 3.21)
project(wav_to_rs485 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#fetchcontent for simpleble
include(FetchContent)

FetchContent_Declare(
    simpleble
    GIT_REPOSITORY https://github.com/OpenBluetoothToolbox/SimpleBLE.git
    GIT_TAG main
)

FetchContent_MakeAvailable(simpleble)

include(${simpleble_SOURCE_DIR}/cmake/prelude.cmake)
include(${simpleble_SOURCE_DIR}/cmake/epilogue.cmake)

#Build the simpleble library using add_subdirectory
add_subdirectory(${simpleble_SOURCE_DIR}/simpleble ${CMAKE_BINARY_DIR}/simpleble)


# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Create the wav_parser executable
add_executable(wav_parser main.cpp serial.cpp wav_parsing.cpp cobs.cpp ble.cpp)

target_link_libraries(wav_parser simpleble::simpleble-c)

# Set output directory
set_target_properties(wav_parser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)