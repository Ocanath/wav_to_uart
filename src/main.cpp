#define DR_WAV_IMPLEMENTATION
#include "args.h"
#include "wav_parsing.h"
#include "serial.h"
#include "cobs.h"
#include <stdio.h>
#include <stdlib.h>
#include "ble.h"
#include "cobs.h"
#include "dartt.h"
#include "dartt_sync.h"
#include "audio.h"

#define BITS_PER_FRAME	40	//2 bytes header, 2 bytes payload, 10 bits per byte = 40 bits

enum {WRITE_FIRST_HALF, WRITE_SECOND_HALF};

/*
	Helper function to wrap a write message and write it out.
	We do this for more control - dartt_sync is great
*/
int write_audio_block(misc_write_message_t &wmsg, dartt_buffer_t &txbuf, Serial & ser)
{
	int rc = dartt_create_write_frame(&wmsg, TYPE_SERIAL_MESSAGE, &txbuf);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}

	cobs_buf_t cb = {.buf = txbuf.buf, .size = txbuf.size, .length = txbuf.len, .encoded_state = COBS_DECODED};
	rc = cobs_encode_single_buffer(&cb);
	if(rc != COBS_SUCCESS)
	{
		return rc;
	}
	ser.write(cb.buf, cb.length);
	return 0;
}

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
    args_t args = parse_args(argc, argv);

    drwav wav;
    if (!drwav_init_file(&wav, args.filename, NULL)) {
        printf("Failed to open WAV file: %s\n", args.filename);
        return 1;
    }

	Serial ser;
	ser.autoconnect(args.baudrate);

    printf("Successfully opened: %s\n\n", args.filename);
    
    // Print file information
    print_wav_info(&wav);
    if(wav.translatedFormatTag != DR_WAVE_FORMAT_PCM || wav.bitsPerSample != 16 || wav.channels != 1)
	{
		printf("Error: re-encode to 16bit, mono (single channel) PCM encoding\n");
		return 1;
	}
	
	//TODO: add feasibility check based on sample rate and baudrate. must be able to write a buffer faster than it gets written out!
	//if(wav.sampleRate != ser.get_baud_rate()/BITS_PER_FRAME)
	//{
	//	printf("Error: use ffmpeg to re-encode with %u", ser.get_baud_rate()/BITS_PER_FRAME);
	//	return 1;
	//}
    // Print sample data

	audio_renderer_t renderer = {};	//control struct for slave renderer


	//update sample rate on the peripheral
	misc_write_message_t write_msg_samplerate = {
		.address = dartt_get_complementary_address(args.dartt_address),
		.index = (uint16_t) (index_of_field(&renderer.retransmission_us, &renderer, sizeof(renderer)) + args.dartt_index),
		.payload = {
			.buf = (unsigned char *)&renderer.retransmission_us,	
			.size = sizeof(renderer.retransmission_us),	//should be 4
			.len = sizeof(renderer.retransmission_us)	//should be 4
		}
	};

	//update first half of render buf on the peripheral
	misc_write_message_t write_msg_lowerhalf =
	{
			.address = dartt_get_complementary_address(args.dartt_address),
			.index = (uint16_t) (index_of_field(&renderer.recv_buffer, &renderer, sizeof(renderer)) + args.dartt_index),
			.payload = {
					.buf = (unsigned char * )renderer.recv_buffer,
					.size = sizeof(renderer.recv_buffer) / 2,
					.len = sizeof(renderer.recv_buffer) / 2
			}
	};

	//update second half of render buf on the peripheral
	misc_write_message_t write_msg_upperhalf =
	{
			.address = dartt_get_complementary_address(args.dartt_address),
			.index = (uint16_t)(index_of_field(&renderer.recv_buffer, &renderer, sizeof(renderer)) + args.dartt_index),
			.payload = {
					.buf = (unsigned char * )renderer.recv_buffer + sizeof(renderer.recv_buffer)/2,
					.size = sizeof(renderer.recv_buffer) / 2,
					.len = sizeof(renderer.recv_buffer) / 2
			}
	};

	unsigned char serialtxbuf[64] = {};
	dartt_buffer_t dartt_serial_tx = { .buf = serialtxbuf, .size = sizeof(serialtxbuf), .len = 0};
	

	int wpending = WRITE_FIRST_HALF;	
    for (drwav_uint64 i = 0; i < wav.totalPCMFrameCount; i++)
    {
		int16_t wav_sample = 0;
		int numwritten = drwav_read_pcm_frames(&wav, 1, &wav_sample);
		printf("%d\n", wav_sample);
		
		// if(audio_buf.len + 2 <= audio_buf.size)
        // {
		// 	audio_buf.len += 
		// }
		// // printf("len = %ld\n", audio_buf.len);
		// if(audio_buf.len >= audio_buf.size)
		// {
		// 	audio_buf.len = 0;
		// }
		
    }
    

    
    drwav_uninit(&wav);
    
    printf("\nDone!\n");
    return 0;
}