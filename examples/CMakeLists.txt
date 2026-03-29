add_executable(dartt_wav_player
    playback.cpp
    args.cpp
)

target_include_directories(dartt_wav_player PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dartt_wav_player PRIVATE dartt_audio)

set_target_properties(dartt_wav_player PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
