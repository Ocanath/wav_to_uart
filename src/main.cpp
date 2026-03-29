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
#include "dartt_init.h"
#include "audio.h"

#define BITS_PER_FRAME	40	//2 bytes header, 2 bytes payload, 10 bits per byte = 40 bits

enum {UNWRITTEN, WRITTEN_UNVISITED, PLAYBACK_IN_PROGRESS, PLAYBACK_DONE};
enum {BPOS_LOWER, BPOS_UPPER};

typedef struct target_buffer_label_t
{
	uint8_t lower_half;
	uint8_t upper_half;
}target_buffer_label_t;	


int load_block(dartt_buffer_t &wmsg, drwav& wav, drwav_uint64 & sample_idx)
{
	for(wmsg.len = 0; wmsg.len < wmsg.size;)
	{
		int16_t wav_sample = 0;
		int numwritten = drwav_read_pcm_frames(&wav, 1, &wav_sample);
		if(numwritten == 0)
		{
			return 1;
		}

		sample_idx++;
		wmsg.buf[wmsg.len++] = (unsigned char)(wav_sample & 0x00FF);
		wmsg.buf[wmsg.len++] = (unsigned char)((wav_sample & 0xFF00) >> 8);
	}
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

	dartt_sync_t ds = {};
	init_ds(ds, ser);
	ds.address = args.dartt_address;
	ds.base_offset = args.dartt_index;

	audio_renderer_t renderer_ctl = {};	//control struct for slave renderer
	audio_renderer_t renderer_shadow = {};
	ds.ctl_base.buf = (unsigned char *)(&renderer_ctl);
	ds.ctl_base.size = sizeof(renderer_ctl);
	ds.periph_base.buf = (unsigned char *)(&renderer_shadow);
	ds.periph_base.size = sizeof(renderer_shadow);

	//update sample rate on the peripheral
	dartt_buffer_t samplerate = {
		.buf = (unsigned char *)(&renderer_ctl.retransmission_us),
		.size = sizeof(renderer_ctl.retransmission_us),
		.len = sizeof(renderer_ctl.retransmission_us)
	};

	//for updating the buffer position. one-time write
	dartt_buffer_t bufferposition = {
		.buf = (unsigned char *)(&renderer_ctl.buffer_pos),
		.size = sizeof(renderer_ctl.buffer_pos),
		.len = sizeof(renderer_ctl.buffer_pos)
	};

	size_t audiobuf_nsamples = sizeof(renderer_ctl.recv_buffer)/sizeof(int16_t);
	//update first half of render buf on the peripheral
	dartt_buffer_t lowerhalf = {
		.buf = (unsigned char *)(&renderer_ctl.recv_buffer[0]),
		.size = sizeof(renderer_ctl.recv_buffer) / 2,
		.len = sizeof(renderer_ctl.recv_buffer) / 2
	};


	//update second half of render buf on the peripheral
	dartt_buffer_t upperhalf = {
		.buf = (unsigned char *)(&renderer_ctl.recv_buffer[audiobuf_nsamples/2]),
		.size = sizeof(renderer_ctl.recv_buffer) / 2,
		.len = sizeof(renderer_ctl.recv_buffer) / 2
	};


	target_buffer_label_t target_tracker = {
		.lower_half = UNWRITTEN,
		.upper_half = UNWRITTEN
	};
	drwav_uint64 sample_idx = 0;

	//initialize - stop playback
	renderer_ctl.buffer_pos = 0;
	renderer_ctl.retransmission_us = 0;
	dartt_write_multi(&bufferposition, &ds);
	dartt_write_multi(&samplerate, &ds);

	while(sample_idx < wav.totalPCMFrameCount)
    {
		if(target_tracker.lower_half == UNWRITTEN && target_tracker.upper_half == UNWRITTEN)
		{
			load_block(lowerhalf, wav, sample_idx);
			dartt_write_multi(&lowerhalf, &ds);
			target_tracker.lower_half = PLAYBACK_IN_PROGRESS;
			load_block(upperhalf, wav, sample_idx);
			dartt_write_multi(&upperhalf, &ds);
			target_tracker.upper_half = WRITTEN_UNVISITED;

			//start playback by writing nonzero retransmission period
			renderer_ctl.retransmission_us = 1000000/wav.sampleRate;
			dartt_write_multi(&samplerate, &ds);
		}
		
		// read_playback_idx(rmsg_pos, dartt_serial_tx, cobs_serial_rx, renderer, ser);
		dartt_read_multi(&bufferposition, &ds);
		uint8_t bpos_region;
		if(renderer_shadow.buffer_pos < (sizeof(renderer_shadow.recv_buffer)/sizeof(int16_t)) / 2)
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
			load_block(lowerhalf, wav, sample_idx);
			dartt_write_multi(&lowerhalf, &ds);
		}
		else if(target_tracker.upper_half == PLAYBACK_IN_PROGRESS && bpos_region == BPOS_LOWER)
		{
			target_tracker.upper_half = PLAYBACK_DONE;
			target_tracker.lower_half = PLAYBACK_IN_PROGRESS;
			load_block(upperhalf, wav, sample_idx);
			dartt_write_multi(&upperhalf, &ds);
		}
		
    }
    
	renderer_ctl.retransmission_us = 0;
	dartt_write_multi(&samplerate, &ds);

    
    drwav_uninit(&wav);
    
    printf("\nDone!\n");
    return 0;
}