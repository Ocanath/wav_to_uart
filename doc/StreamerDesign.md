# Audio Streamer over DARTT

## Introduction

This code is an update from a basic audio streamer that relied on UART encoding timing to sample audio. It required a .wav file re-sampled **specifically** at the target bitrate as input and simply blasted out COBS stuffed frames. These were grouped into larger frames before transmitted out via serial in order for the transmission to be 'gapless'. This worked well - however it is problematic for a couple reasons:

1. Error correction: frames were not signed
2. Unnecessarily high overhead - each individual wav frame had 2 cobs framing bytes, meaning the overhead was 50% of the traffic. 

The new system will rely on dartt. The assumption is that any dartt-supported device which has the `audio_renderer_t`. It is agnostic to the full dartt control blob - it assumes only that the dartt structure has the `audio_renderer_t` structure somewhere within it, and the caller to the program sets the location.

## Audio over dartt

The new audio streaming over dartt concept is relatively simple. It uses a double buffer approach, where one half of recv_buffer is actively getting written out as audio, and the other half of recv_buffer is getting written into via DARTT over serial.

A requirement for this to function properly is that amount of time required to *consume* one half of the buffer must be greater than the amount of time required to *add* new data to the other half.  

The audio streaming service does the following:
- Read the sampling rate of the input wav file
- Determine whether it is possible to stream as a function of the sampling rate, baudrate, average write/read time, and buffer size. If the time to playback the audio is greater than the time to poll and write the unused half of the buffer, the file is renderable.
- Writes the appropriate sample period to the peripheral
- Streams the data. This works by first transmitting both chunks, then after the first full block write, polling the buffer_pos. If the buffer_pos has entered the second half after writing the first half, the first half is written with new data. If the second half has finished and wraps back around to the first half, the first half is written with new data. 

Playback is stopped by setting the retransmission period to 0.


The peripheral device does the following:

- Playback works according to `audio_stream()` in `audio.c`. 
- When `retransmission_us` is zero, it is idle (does not play anything).
- When `retransmission_us` is nonzero, it immediately begins playback
- A new sample is loaded out from the recv_buffer to the speaker once every `retransmission_us` microseconds.
- The buffer is circular - once it reaches the end, it loops back around to the beginning


