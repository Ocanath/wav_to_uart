### Utility to send audio over UART

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


## Linking for Embedded

The `embedded/` directory is meant to be submoduled into whatever embedded build compilation toolchain you have. I.e.  for stm32cubide, the recommended flow is:

1. submodule this repo
1. update the .project, .cproject settings with external/dartt_audio/embedded/ - i.e. link audio.c and audio.h from the submoduled repo

That's it!

