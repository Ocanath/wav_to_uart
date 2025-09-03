#define DR_WAV_IMPLEMENTATION
#include "wav_parsing.h"
#include "serial.h"
#include <stdio.h>
#include <stdlib.h>

#define BITS_PER_FRAME	40	//2 bytes header, 2 bytes payload, 10 bits per byte = 40 bits

int main(int argc, char** argv) 
{
    if (argc < 2) {
        printf("Usage: %s <wav_file>\n", argv[0]);
        printf("Example: %s audio.wav\n", argv[0]);
        return 1;
    }
        
    drwav wav;
    if (!drwav_init_file(&wav, argv[1], NULL)) {
        printf("Failed to open WAV file: %s\n", argv[1]);
        return 1;
    }
    
	Serial ser;
	ser.autoconnect(921600);

    printf("Successfully opened: %s\n\n", argv[1]);
    
    // Print file information
    print_wav_info(&wav);
    if(wav.translatedFormatTag != DR_WAVE_FORMAT_PCM || wav.bitsPerSample != 16)
	{
		printf("Error: re-encode to 16bit, mono (single channel) PCB encoding\n");
		return 1;
	}
	if(wav.sampleRate != ser.get_baud_rate()/BITS_PER_FRAME)
	{
		printf("Error: use ffmpeg to re-encode with %u", ser.get_baud_rate()/BITS_PER_FRAME);
		return 1;
	}
    // Print sample data
    
    
    drwav_uninit(&wav);
    
    printf("\nDone!\n");
    return 0;
}