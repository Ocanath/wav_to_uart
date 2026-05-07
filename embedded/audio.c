#include "audio.h"

static uint32_t prev_us = 0;
/*
 * Inputs:
 * a: pointer to audio streamer control structure. Contains playback sampling rate
 * t_us: time in microseconds. This overflows - program relies on unsigned subtraction for proper delta
 * */
int32_t audio_stream(audio_renderer_t * a, uint32_t t_us)
{
	if(a == NULL)
	{
		return 0;
	}
	if(a->retransmission_us == 0)
	{
		prev_us = t_us;
		return 0;
	}

	if(t_us - prev_us >= a->retransmission_us)
	{
		prev_us = t_us;
		uint32_t bufsize = (sizeof(a->recv_buffer)/sizeof(int16_t));
		a->buffer_pos = (a->buffer_pos + 1) % bufsize;
		return a->recv_buffer[a->buffer_pos];
	}
	else
	{
		if(a->buffer_pos >= (sizeof(a->recv_buffer)/sizeof(int16_t)))
		{
			return 0;
		}
		return a->recv_buffer[a->buffer_pos];
	}
}

/** @brief simpler playback mechanism. Plays half buffers out of a, stopping when the end is reached. 
 * Half buffers are switched based on block_idx
 * 
 */
int32_t audio_stream_nosync(audio_renderer_t * a, int block_idx, uint32_t t_us)
{
	if(a == NULL)
	{
		return 0;
	}
	if(a->retransmission_us == 0)
	{
		prev_us = t_us;
		return 0;
	}

	const size_t half_buffersize = (sizeof(a->recv_buffer)/sizeof(int16_t)) / 2;
	uint32_t block_start = (uint32_t)block_idx * half_buffersize;
	uint32_t block_end = block_start + half_buffersize;

	if(a->buffer_pos < block_start || a->buffer_pos > block_end)
	{
		a->buffer_pos = block_start;
	}

	if(a->buffer_pos >= block_end)
	{
		return 0;
	}

	if(t_us - prev_us >= a->retransmission_us)
	{
		prev_us = t_us;
		a->buffer_pos++;
		if(a->buffer_pos >= block_end)
		{
			return 0;
		}
	}
	return a->recv_buffer[a->buffer_pos];
}

