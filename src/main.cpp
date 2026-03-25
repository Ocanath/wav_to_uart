#define DR_WAV_IMPLEMENTATION
#include "wav_parsing.h"
#include "serial.h"
#include "cobs.h"
#include <stdio.h>
#include <stdlib.h>
#include "ble.h"
#include "cobs.h"
#include "dartt.h"
#include "audio.h"

#define BITS_PER_FRAME	40	//2 bytes header, 2 bytes payload, 10 bits per byte = 40 bits


/*

TODO: Develop the BLE integrations
	1. Flag parsing to check for --ble flag, with --name (filter), --connect-by-name(?) and --mac options to skip the command line interactive scanner
	2. flesh out the scanner - include interactive option (connect or notify examples!), name filtering, or direct mac 
	3. need to subscribe to the notification service
	4. need to test writing and receiving
	5. Need to design the data transfer! Current concept:
		-MTU double buffer on the embedded side
		-transfer an MTU. the embedded begins playback via UART. With enough time left in the current buffer playback for a new MTU transfer, the embedded system will trigger a notification. 
		-Upon reception of that notification, a new MTU will be transferred, and the client will wait for another notification before transferring the next one.
	Potential issues:
		-notification latency or dropped frames leading to skips, drops, or complete failure. 
		-Some back of envelope analysis: an MTU is 247 bytes. That means 246 bytes -> 123 frames. At an encoding of 17631 frames per second, that means an MTU will buffer out in 6.97ms.
		The minimum connection interval is 7.5ms, which means... YOU'RE FUCKED because you need to send data faster than that. rip
*/

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

	audio_renderer_t renderer = {};	//control struct for slave renderer
	dartt_buffer_t audio_buf = {
		.buf = (unsigned char *)&renderer.recv_buffer,
		.size = sizeof(renderer.recv_buffer), 
		.len = 0 
	};

    for (drwav_uint64 i = 0; i < wav.totalPCMFrameCount; i++)
    {
        audio_buf.len = drwav_read_pcm_frames(&wav, audio_buf.size, audio_buf.buf);
		printf("len = %ld\n", audio_buf.len);
    }
    

    
    drwav_uninit(&wav);
    
    printf("\nDone!\n");
    return 0;
}