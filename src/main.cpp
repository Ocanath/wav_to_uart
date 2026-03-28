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

enum {UNWRITTEN, WRITTEN_UNVISITED, PLAYBACK_IN_PROGRESS, PLAYBACK_DONE};
enum {BPOS_LOWER, BPOS_UPPER};

typedef struct target_buffer_label_t
{
	uint8_t lower_half;
	uint8_t upper_half;
}target_buffer_label_t;	


int load_block(misc_write_message_t &wmsg, drwav& wav, drwav_uint64 & sample_idx)
{
	for(wmsg.payload.len = 0; wmsg.payload.len < wmsg.payload.size;)
	{
		int16_t wav_sample = 0;
		int numwritten = drwav_read_pcm_frames(&wav, 1, &wav_sample);
		if(numwritten == 0)
		{
			return 0;
		}
		
		sample_idx++;
		wmsg.payload.buf[wmsg.payload.len++] = (unsigned char)(wav_sample & 0x00FF);
		wmsg.payload.buf[wmsg.payload.len++] = (unsigned char)((wav_sample & 0xFF00) >> 8);
	}
	return 0;
}

/*
	Helper function to wrap a write message and write it out.
	We do this for more control - dartt_sync is great
*/
int write_audio_data(misc_write_message_t &wmsg, dartt_buffer_t &txbuf, Serial & ser)
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
Steps:
1. take rmsg, create payload buffer
2. cobs encode payload buffer
3. send it out over serial
4. recieve cobs encoded dartt reply
5. cobs decode reply
6. dartt decode reply into renderer buffer
7. profit??

*/
int read_playback_idx(
	misc_read_message_t & rmsg,
	dartt_buffer_t &txbuf,
	cobs_buf_t & rxbuf,
	audio_renderer_t & renderer,
	Serial & ser
	)
{
	int rc = dartt_create_read_frame(&rmsg, TYPE_SERIAL_MESSAGE, &txbuf);
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

	rc = ser.read_until_delimiter(rxbuf.buf, rxbuf.size, 0, 10);		//a timeout will completely fuck playback, but that's ok
	if(rc > 0)
	{
		rxbuf.length = rc;
	}
	else
	{
		return rc;
	}
	
	unsigned char rxdecode_mem[128];

	cobs_buf_t bufferidx_alias = {
		.buf = rxdecode_mem, 
		.size = sizeof(rxdecode_mem), 
		.length = 0, 
		.encoded_state = COBS_DECODED
	};
	rc = cobs_decode_double_buffer(&rxbuf, &bufferidx_alias);
	if(rc != COBS_SUCCESS)
	{
		return rc;
	}

	dartt_buffer_t msg_alias = {
		.buf = rxdecode_mem,
		.size = sizeof(rxdecode_mem),
		.len = 0
	};
	payload_layer_msg_t pld_msg = {};
	rc = dartt_frame_to_payload(&msg_alias, TYPE_SERIAL_MESSAGE, PAYLOAD_ALIAS, &pld_msg);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}
	const dartt_mem_t dest = {
		.buf = (unsigned char *)(&renderer.buffer_pos),
		.size = sizeof(renderer.buffer_pos),
	};
	rc = dartt_parse_read_reply(&pld_msg, &rmsg, &dest);
	return rc;
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

	//for updating the buffer position. one-time write
	misc_write_message_t write_msg_bufferposition = {
		.address = dartt_get_complementary_address(args.dartt_address),
		.index = (uint16_t)(index_of_field(&renderer.buffer_pos, &renderer, sizeof(renderer)) + args.dartt_index),
		.payload = {
			.buf = (unsigned char *)(&renderer.buffer_pos),
			.size = sizeof(renderer.buffer_pos),
			.len = sizeof(renderer.buffer_pos)
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

	misc_read_message_t rmsg_pos = {
		.address = args.dartt_address,
		.index = (uint16_t)(index_of_field(&renderer.buffer_pos, &renderer, sizeof(renderer)) + args.dartt_index),
		.num_bytes = sizeof(renderer.buffer_pos)
	};

	unsigned char serialtxbuf[128] = {};
	dartt_buffer_t dartt_serial_tx = { .buf = serialtxbuf, .size = sizeof(serialtxbuf), .len = 0};
	unsigned char serialrxbuf[128] = {};
	cobs_buf_t cobs_serial_rx = { .buf = serialrxbuf, .size = sizeof(serialrxbuf), .length = 0, .encoded_state = COBS_ENCODED};
	

	target_buffer_label_t target_tracker = {
		.lower_half = UNWRITTEN,
		.upper_half = UNWRITTEN
	};
	drwav_uint64 sample_idx = 0;

	//initialize - stop playback
	renderer.buffer_pos = 0;
	renderer.retransmission_us = 0;
	write_audio_data(write_msg_bufferposition, dartt_serial_tx, ser);
	write_audio_data(write_msg_samplerate, dartt_serial_tx, ser);

	while(sample_idx < wav.totalPCMFrameCount)
    {
		if(target_tracker.lower_half == UNWRITTEN && target_tracker.upper_half == UNWRITTEN)
		{
			load_block(write_msg_lowerhalf, wav, sample_idx);
			write_audio_data(write_msg_lowerhalf, dartt_serial_tx, ser);
			target_tracker.lower_half = PLAYBACK_IN_PROGRESS;
			load_block(write_msg_upperhalf, wav, sample_idx);
			write_audio_data(write_msg_upperhalf, dartt_serial_tx, ser);
			target_tracker.upper_half = WRITTEN_UNVISITED;

			//start playback by writing nonzero retransmission period
			renderer.retransmission_us = 1000000/wav.sampleRate;	
			write_audio_data(write_msg_samplerate, dartt_serial_tx, ser);
		}
		
		read_playback_idx(rmsg_pos, dartt_serial_tx, cobs_serial_rx, renderer, ser);
		uint8_t bpos_region;
		if(renderer.buffer_pos < sizeof(renderer.recv_buffer)/sizeof(int16_t))
		{
			bpos_region = BPOS_LOWER;
		}
		else
		{
			bpos_region = BPOS_UPPER;
		}

		if(target_tracker.lower_half == PLAYBACK_IN_PROGRESS && bpos_region == BPOS_UPPER)
		{
			target_tracker.lower_half = PLAYBACK_DONE;
			target_tracker.upper_half = PLAYBACK_IN_PROGRESS;
			load_block(write_msg_lowerhalf, wav, sample_idx);
			write_audio_data(write_msg_lowerhalf, dartt_serial_tx, ser);
		}
		else if(target_tracker.upper_half == PLAYBACK_IN_PROGRESS && bpos_region == BPOS_LOWER)
		{
			target_tracker.upper_half = PLAYBACK_DONE;
			target_tracker.lower_half = PLAYBACK_IN_PROGRESS;
			load_block(write_msg_upperhalf, wav, sample_idx);
			write_audio_data(write_msg_upperhalf, dartt_serial_tx, ser);
		}
		
    }
    

    
    drwav_uninit(&wav);
    
    printf("\nDone!\n");
    return 0;
}