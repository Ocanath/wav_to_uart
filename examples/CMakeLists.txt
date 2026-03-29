add_executable(example_playback
    example_playback.cpp
    args.cpp
)

target_include_directories(example_playback PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(example_playback PRIVATE dartt_audio)

set_target_properties(example_playback PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
