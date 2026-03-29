#include "AudioWriter.h"

/*
TODO: move the callbacks fully into this scope, hiding it from other TU's
This means hiding sizes, dynamically allocating the tx and rx buffers, a destructor
*/


AudioWriter::AudioWriter(unsigned char addr, uint32_t dartt_offset, Serial & ser)
{
	ds.address = addr;
	ds.base_offset = dartt_offset;
	ds.msg_type = TYPE_SERIAL_MESSAGE;
	ds.tx_buf.buf = tx_buf_mem;
	ds.tx_buf.size = sizeof(tx_buf_mem) - NUM_BYTES_COBS_OVERHEAD;		//DO NOT CHANGE. This is for a good reason. See above note
	ds.tx_buf.len = 0;
	ds.rx_buf.buf = rx_buf_mem;
	ds.rx_buf.size = sizeof(rx_buf_mem) - NUM_BYTES_COBS_OVERHEAD;	//DO NOT CHANGE. This is for a good reason. See above note
	ds.rx_buf.len = 0;
	ds.blocking_tx_callback = &tx_blocking;
	ds.user_context_tx = (void*)(&ser);
	ds.blocking_rx_callback = &rx_blocking;
	ds.user_context_rx = (void*)(&ser);
	ds.timeout_ms = 10;

	ds.ctl_base.buf = (unsigned char *)(&renderer_ctl);	//must be assigned
	ds.ctl_base.size = sizeof(audio_renderer_t);

	ds.periph_base.buf = (unsigned char *)(&renderer_shadow);	//must be assigned
	ds.periph_base.size = sizeof(audio_renderer_t);

	for(size_t i = 0; i < sizeof(audio_renderer_t); i++)
	{
		ds.ctl_base.buf[i] = 0;
		ds.periph_base.buf[i] = 0;
	}

	//update sample rate on the peripheral
	samplerate = {
		.buf = (unsigned char *)(&renderer_ctl.retransmission_us),
		.size = sizeof(renderer_ctl.retransmission_us),
		.len = sizeof(renderer_ctl.retransmission_us)
	};

		//for updating the buffer position. one-time write
	bufferposition = {
		.buf = (unsigned char *)(&renderer_ctl.buffer_pos),
		.size = sizeof(renderer_ctl.buffer_pos),
		.len = sizeof(renderer_ctl.buffer_pos)
	};

	size_t audiobuf_nsamples = sizeof(renderer_ctl.recv_buffer)/sizeof(int16_t);
	//update first half of render buf on the peripheral
	lowerhalf = {
		.buf = (unsigned char *)(&renderer_ctl.recv_buffer[0]),
		.size = sizeof(renderer_ctl.recv_buffer) / 2,
		.len = sizeof(renderer_ctl.recv_buffer) / 2
	};

	//update second half of render buf on the peripheral
	upperhalf = {
		.buf = (unsigned char *)(&renderer_ctl.recv_buffer[audiobuf_nsamples/2]),
		.size = sizeof(renderer_ctl.recv_buffer) / 2,
		.len = sizeof(renderer_ctl.recv_buffer) / 2
	};
}

int AudioWriter::play(const char * filename)
{
	if (!drwav_init_file(&wav, filename, NULL)) {
        printf("Failed to open WAV file: %s\n", filename);
        return 1;
    }
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

	target_buffer_label_t target_tracker = {
		.lower_half = UNWRITTEN,
		.upper_half = UNWRITTEN
	};

		drwav_uint64 sample_idx = 0;

	//initialize - stop playback
	renderer_ctl.buffer_pos = 0;
	renderer_ctl.retransmission_us = 0;
	int rc = 0;
	rc = dartt_write_multi(&bufferposition, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}
	rc = dartt_write_multi(&samplerate, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}

	while(sample_idx < wav.totalPCMFrameCount)
    {
		if(target_tracker.lower_half == UNWRITTEN && target_tracker.upper_half == UNWRITTEN)
		{
			load_block(lowerhalf, sample_idx);
			dartt_write_multi(&lowerhalf, &ds);
			target_tracker.lower_half = PLAYBACK_IN_PROGRESS;
			load_block(upperhalf, sample_idx);
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
			load_block(lowerhalf, sample_idx);
			dartt_write_multi(&lowerhalf, &ds);
		}
		else if(target_tracker.upper_half == PLAYBACK_IN_PROGRESS && bpos_region == BPOS_LOWER)
		{
			target_tracker.upper_half = PLAYBACK_DONE;
			target_tracker.lower_half = PLAYBACK_IN_PROGRESS;
			load_block(upperhalf, sample_idx);
			dartt_write_multi(&upperhalf, &ds);
		}		
    }

	renderer_ctl.retransmission_us = 0;
	dartt_write_multi(&samplerate, &ds);

	drwav_uninit(&wav);
	return 0;
}

AudioWriter::~AudioWriter()
{

}

int AudioWriter::load_block(dartt_buffer_t &wmsg, drwav_uint64 & sample_idx)
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
