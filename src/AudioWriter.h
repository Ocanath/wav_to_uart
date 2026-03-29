#ifndef AUDIO_WRITER_H
#define AUDIO_WRITER_H
#include "dartt_sync.h"
#include "serial.h"
#include "audio.h"
#include "wav_parsing.h"

enum {UNWRITTEN, WRITTEN_UNVISITED, PLAYBACK_IN_PROGRESS, PLAYBACK_DONE};
enum {BPOS_LOWER, BPOS_UPPER};

typedef struct target_buffer_label_t
{
	uint8_t lower_half;
	uint8_t upper_half;
}target_buffer_label_t;	


class AudioWriter
{
	public:
		AudioWriter(unsigned char addr, 
			uint32_t dartt_offset,
			Serial & ser
		);
		~AudioWriter();
		int play(const char * filename);
	private:
		audio_renderer_t renderer_ctl;
		audio_renderer_t renderer_shadow;		
		dartt_sync_t ds;
		unsigned char * tx_buf_mem;
		unsigned char * rx_buf_mem;

		//update sample rate on the peripheral
		dartt_buffer_t samplerate;
		//for updating the buffer position. one-time write
		dartt_buffer_t bufferposition;
		//update first half of render buf on the peripheral
		dartt_buffer_t lowerhalf;
		//update second half of render buf on the peripheral
		dartt_buffer_t upperhalf;

		drwav wav;	
		int load_block(dartt_buffer_t &wmsg, drwav_uint64 & sample_idx);
};


#endif
