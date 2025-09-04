#define DR_WAV_IMPLEMENTATION
#include "wav_parsing.h"
#include "serial.h"
#include "cobs.h"
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
    if(wav.translatedFormatTag != DR_WAVE_FORMAT_PCM || wav.bitsPerSample != 16 || wav.channels != 1)
	{
		printf("Error: re-encode to 16bit, mono (single channel) PCM encoding\n");
		return 1;
	}
	//if(wav.sampleRate != ser.get_baud_rate()/BITS_PER_FRAME)
	//{
	//	printf("Error: use ffmpeg to re-encode with %u", ser.get_baud_rate()/BITS_PER_FRAME);
	//	return 1;
	//}
    // Print sample data
    cobs_buf_t msg = {};
    unsigned char msgbuf[8] = {};   //actually 4, whatever
    msg.buf = msgbuf;
    msg.size = sizeof(msgbuf);

    unsigned char tx_multiple[4 * 8] = {};
    int tx_buffer_idx = 0;
    for (int i = 0; i < wav.totalPCMFrameCount; i++)
    {
        msg.length = drwav_read_pcm_frames(&wav, 1, msg.buf)*sizeof(int16_t);
        cobs_encode_single_buffer(&msg);
        for (int midx = 0; midx < msg.length; midx++)
        {
            tx_multiple[tx_buffer_idx++] = msg.buf[midx];
            if (tx_buffer_idx >= sizeof(tx_multiple))
            {
                ser.write(tx_multiple, tx_buffer_idx);
                tx_buffer_idx = 0;
            }
        }
    }
    

    
    drwav_uninit(&wav);
    
    printf("\nDone!\n");
    return 0;
}