#include "AudioWriter.h"
#include "cobs.h"

/*
TODO: move the callbacks fully into this scope, hiding it from other TU's
This means hiding sizes, dynamically allocating the tx and rx buffers, a destructor
*/
#define _AUDIO_SERIAL_BUFFER_SIZE 32
#define _AUDIO_NUM_BYTES_COBS_OVERHEAD	2	//give cobs room to operate


int _audio_tx_blocking(unsigned char addr, dartt_buffer_t * b, void * user_context, uint32_t timeout)
{
	if(user_context == NULL)
	{
		return -2;
	}
	Serial * pser = (Serial*)(user_context);
	if(pser->connected() == false)
	{
		return -2;
	}
	cobs_buf_t cb = {
		.buf = b->buf,
		.size = _AUDIO_SERIAL_BUFFER_SIZE,	//important! this has to be the true size - b doesn't know what size it actually has. Use SERIAL_BUFFER_SIZE as source of truth for size since sizeof(var) is out of scope
		.length = b->len,
		.encoded_state = COBS_DECODED
	};
	int rc = cobs_encode_single_buffer(&cb);
	if (rc != 0)
	{
		return rc;
	}

	rc = pser->write(cb.buf, (int)cb.length);
	if(rc == (int)cb.length)
	{
		return DARTT_PROTOCOL_SUCCESS;
	}
	else
	{
		return -1;
	}
}

int _audio_rx_blocking(dartt_buffer_t * buf, void * user_context, uint32_t timeout)
{
	if(user_context == NULL)
	{
		return -2;
	}
	Serial * pser = (Serial*)(user_context);
	if(pser->connected() == false)
	{
		return -2;
	}

	cobs_buf_t cb_enc =
	{
		.buf = buf->buf,
		.size = _AUDIO_SERIAL_BUFFER_SIZE,	//important! this has to be the true size - b doesn't know what size it actually has. Use SERIAL_BUFFER_SIZE as source of truth for size since sizeof(var) is out of scope
		.length = 0
	};

	int rc;
	rc = pser->read_until_delimiter(cb_enc.buf, cb_enc.size, 0, timeout);

	if (rc >= 0)
	{
		cb_enc.length = rc;	//load encoded length (raw buffer)
	}
	else if (rc == -2)
	{
		return -7;
	}
	else
	{
		return -1;
	}

	cobs_buf_t cb_dec =
	{
		.buf = buf->buf,
		.size = buf->size,
		.length = 0
	};
	rc = cobs_decode_double_buffer(&cb_enc, &cb_dec);
	buf->len = cb_dec.length;	//critical - we are aliasing this read buffer in sync, but must update the length to the cobs decoded value

	if (rc != COBS_SUCCESS)
	{
		return rc;
	}
	else
	{
		return DARTT_PROTOCOL_SUCCESS;
	}
    
}


AudioWriter::AudioWriter(unsigned char addr, uint32_t dartt_offset, Serial & ser)
{
	tx_buf_mem = new unsigned char[_AUDIO_SERIAL_BUFFER_SIZE];
	rx_buf_mem = new unsigned char[_AUDIO_SERIAL_BUFFER_SIZE];

	ds.address = addr;
	ds.base_offset = dartt_offset;
	ds.msg_type = TYPE_SERIAL_MESSAGE;
	ds.tx_buf.buf = tx_buf_mem;
	ds.tx_buf.size = _AUDIO_SERIAL_BUFFER_SIZE - _AUDIO_NUM_BYTES_COBS_OVERHEAD;		//DO NOT CHANGE. This is for a good reason. See above note
	ds.tx_buf.len = 0;
	ds.rx_buf.buf = rx_buf_mem;
	ds.rx_buf.size = _AUDIO_SERIAL_BUFFER_SIZE - _AUDIO_NUM_BYTES_COBS_OVERHEAD;	//DO NOT CHANGE. This is for a good reason. See above note
	ds.rx_buf.len = 0;
	ds.blocking_tx_callback = &_audio_tx_blocking;
	ds.user_context_tx = (void*)(&ser);
	ds.blocking_rx_callback = &_audio_rx_blocking;
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
	};

		//for updating the buffer position. one-time write
	bufferposition = {
		.buf = (unsigned char *)(&renderer_ctl.buffer_pos),
		.size = sizeof(renderer_ctl.buffer_pos),
	};

	size_t audiobuf_nsamples = sizeof(renderer_ctl.recv_buffer)/sizeof(int16_t);
	//update first half of render buf on the peripheral
	lowerhalf = {
		.buf = (unsigned char *)(&renderer_ctl.recv_buffer[0]),
		.size = sizeof(renderer_ctl.recv_buffer) / 2,
	};

	//update second half of render buf on the peripheral
	upperhalf = {
		.buf = (unsigned char *)(&renderer_ctl.recv_buffer[audiobuf_nsamples/2]),
		.size = sizeof(renderer_ctl.recv_buffer) / 2,
	};
}

AudioWriter::~AudioWriter()
{
	delete[] tx_buf_mem;
	delete[] rx_buf_mem;
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
		rc = dartt_read_multi(&bufferposition, &ds);
		if(rc != DARTT_PROTOCOL_SUCCESS)
		{
			break;
		}
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
	return rc;
}


int AudioWriter::load_block(dartt_mem_t &wmsg, drwav_uint64 & sample_idx)
{
	
	for(size_t i = 0; i < wmsg.size;)
	{
		int16_t wav_sample = 0;
		int numwritten = drwav_read_pcm_frames(&wav, 1, &wav_sample);
		if(numwritten == 0)
		{
			return 1;
		}

		sample_idx++;

		if(i + 2 > wmsg.size)
		{
			return DARTT_ERROR_MEMORY_OVERRUN;
		}
		wmsg.buf[i++] = (unsigned char)(wav_sample & 0x00FF);
		wmsg.buf[i++] = (unsigned char)((wav_sample & 0xFF00) >> 8);
	}
	return 0;
}
