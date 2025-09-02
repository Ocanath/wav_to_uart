#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <stdio.h>
#include <stdlib.h>

void print_wav_info(drwav* pWav) {
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

void print_sample_data(drwav* pWav, int max_samples) {
    printf("=== Sample Data Dump ===\n");
    
    if (pWav->bitsPerSample == 16) {
        drwav_int16* pSampleData = malloc(max_samples * sizeof(drwav_int16));
        if (pSampleData == NULL) {
            printf("Failed to allocate memory for sample data\n");
            return;
        }
        
        drwav_uint64 samplesRead = drwav_read_pcm_frames_s16(pWav, max_samples / pWav->channels, pSampleData);
        
        printf("Read %llu samples (16-bit signed):\n", samplesRead * pWav->channels);
        for (int i = 0; i < samplesRead * pWav->channels && i < max_samples; i++) {
            if (pWav->channels == 2) {
                if (i % 2 == 0) {
                    printf("Sample %d: L=%6d, ", i/2, pSampleData[i]);
                } else {
                    printf("R=%6d\n", pSampleData[i]);
                }
            } else {
                printf("Sample %d: %6d\n", i, pSampleData[i]);
            }
        }
        
        free(pSampleData);
    } else if (pWav->bitsPerSample == 32) {
        drwav_int32* pSampleData = malloc(max_samples * sizeof(drwav_int32));
        if (pSampleData == NULL) {
            printf("Failed to allocate memory for sample data\n");
            return;
        }
        
        drwav_uint64 samplesRead = drwav_read_pcm_frames_s32(pWav, max_samples / pWav->channels, pSampleData);
        
        printf("Read %llu samples (32-bit signed):\n", samplesRead * pWav->channels);
        for (int i = 0; i < samplesRead * pWav->channels && i < max_samples; i++) {
            if (pWav->channels == 2) {
                if (i % 2 == 0) {
                    printf("Sample %d: L=%10d, ", i/2, pSampleData[i]);
                } else {
                    printf("R=%10d\n", pSampleData[i]);
                }
            } else {
                printf("Sample %d: %10d\n", i, pSampleData[i]);
            }
        }
        
        free(pSampleData);
    } else {
        // For other bit depths, read as float
        float* pSampleData = malloc(max_samples * sizeof(float));
        if (pSampleData == NULL) {
            printf("Failed to allocate memory for sample data\n");
            return;
        }
        
        drwav_uint64 samplesRead = drwav_read_pcm_frames_f32(pWav, max_samples / pWav->channels, pSampleData);
        
        printf("Read %llu samples (normalized float):\n", samplesRead * pWav->channels);
        for (int i = 0; i < samplesRead * pWav->channels && i < max_samples; i++) {
            if (pWav->channels == 2) {
                if (i % 2 == 0) {
                    printf("Sample %d: L=%8.5f, ", i/2, pSampleData[i]);
                } else {
                    printf("R=%8.5f\n", pSampleData[i]);
                }
            } else {
                printf("Sample %d: %8.5f\n", i, pSampleData[i]);
            }
        }
        
        free(pSampleData);
    }
    
    printf("========================\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <wav_file> [max_samples]\n", argv[0]);
        printf("Example: %s audio.wav 100\n", argv[0]);
        return 1;
    }
    
    int max_samples = 50; // Default to 50 samples
    if (argc >= 3) {
        max_samples = atoi(argv[2]);
        if (max_samples <= 0) max_samples = 50;
    }
    
    drwav wav;
    if (!drwav_init_file(&wav, argv[1], NULL)) {
        printf("Failed to open WAV file: %s\n", argv[1]);
        return 1;
    }
    
    printf("Successfully opened: %s\n\n", argv[1]);
    
    // Print file information
    print_wav_info(&wav);
    
    // Print sample data
    print_sample_data(&wav, max_samples);
    
    drwav_uninit(&wav);
    
    printf("\nDone!\n");
    return 0;
}