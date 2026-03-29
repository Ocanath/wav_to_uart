# dartt_audio

A library for streaming audio to embedded targets over UART using the DARTT protocol.

ffmpeg usage: In order to properly parse the audio file (.wav only) it must be properly encoded. This program expects very simple encoding. It must be MONO, and it should be encoded for a specific baud rate.

In order to determine the re-encoding rate, use the simple formula: `baud/40'. This comes from: two bytes per sample (payload), two bytes header (COBS), 10 bits per byte = 40 bits, baud rate is bits/sec.

So the workflow for 921600 baud playback is:
```
yt-dlp 'url to audio'
ffmpeg -i audio.m4a audio.wav
ffmpeg -i audio.wav -ar 23040 -ac 1 audioReadyToGo.wav
wav-to-uart audioReadyToGo.wav
```
`-ar` alters the sampling rate, `-ac` changes channel count


To scale the volume using ffmpeg to the achievable system range:

`ffmpeg -i input.wav -af "volume=690/32767" -c:a pcm_s16le output.wav`


## Linking (CMake)

`dartt_audio` is designed to be consumed via `add_subdirectory`. It expects
`dartt-protocol`, `serial-cross-platform`, and `byte-stuffing` to be available
as CMake targets. If the parent project has already added those subdirectories,
`dartt_audio` will reuse their targets automatically. If not, it will pull them
from its own `external/` directory.

```cmake
# In your parent CMakeLists.txt:
add_subdirectory(external/dartt-protocol)
add_subdirectory(external/serial-cross-platform)
add_subdirectory(external/byte-stuffing)
add_subdirectory(external/dartt_audio)   # reuses the targets above

target_link_libraries(your_target PRIVATE dartt_audio)
```

Or, if you want `dartt_audio` to manage its own copies of those dependencies:

```cmake
add_subdirectory(external/dartt_audio)
target_link_libraries(your_target PRIVATE dartt_audio)
```

The consumer's include path will automatically pick up `AudioWriter.h`,
`wav_parsing.h`, `dr_wav.h`, and `audio.h`.

Note: `DR_WAV_IMPLEMENTATION` must be defined in exactly one translation unit
in the consuming project before the first include of any `dartt_audio` header
in that TU (see `examples/example_playback.cpp` for the pattern).

## Building the example

```bash
cmake -B build -DDARTT_AUDIO_BUILD_EXAMPLES=ON [-DCMAKE_BUILD_TYPE=Debug]
cmake --build build
./build/bin/example_playback <file.wav> <dartt_address> <dartt_index> [--baudrate <baud>]
```

## Linking for Embedded

The `embedded/` directory is meant to be submoduled into whatever embedded build compilation toolchain you have. I.e. for STM32CubeIDE, the recommended flow is:

1. Submodule this repo
2. Update the `.project`/`.cproject` settings to include `external/dartt_audio/embedded/` — i.e. link `audio.c` and `audio.h` from the submoduled repo

That's it!

