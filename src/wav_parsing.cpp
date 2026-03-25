#include "wav_parsing.h"

void print_wav_info(drwav* pWav) 
{
    printf("=== WAV File Information ===\n");
    printf("Sample Rate:    %u Hz\n", pWav->sampleRate);
    printf("Channels:       %u\n", pWav->channels);
    printf("Bits Per Sample: %u bits\n", pWav->bitsPerSample);
    printf("Total Samples:  %llu\n", pWav->totalPCMFrameCount);
    printf("Duration:       %.2f seconds\n", 
           (double)pWav->totalPCMFrameCount / pWav->sampleRate);
    printf("Data Format:    ");
    
    switch(pWav->translatedFormatTag) {
        case DR_WAVE_FORMAT_PCM:
            printf("PCM\n");
            break;
        case DR_WAVE_FORMAT_IEEE_FLOAT:
            printf("IEEE Float\n");
            break;
        case DR_WAVE_FORMAT_ALAW:
            printf("A-Law\n");
            break;
        case DR_WAVE_FORMAT_MULAW:
            printf("μ-Law\n");
            break;
        default:
            printf("Other (0x%04X)\n", pWav->translatedFormatTag);
    }
    printf("=============================\n\n");
}


