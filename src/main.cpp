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
#include "AudioWriter.h"


int main(int argc, char** argv) 
{
    args_t args = parse_args(argc, argv);

	Serial ser;
	ser.autoconnect(args.baudrate);

	AudioWriter audiowriter(args.dartt_address, args.dartt_index, ser);
	int rc = audiowriter.play(args.filename);
	if(rc == 0)
    {
		printf("\nDone!\n");
	}
	else
	{
		printf("\n Error - something went wrong with playback\n");
	}
    return rc;
}

