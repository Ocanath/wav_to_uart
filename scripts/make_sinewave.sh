#!/bin/bash

ffmpeg -f lavfi -i "sine=frequency=440:sample_rate=31834:duration=10" -af "volume=2000/32767" -c:a pcm_s16le output.wav