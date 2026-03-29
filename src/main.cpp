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
#include "serial_callbacks.h"
#include "AudioWriter.h"

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
    args_t args = parse_args(argc, argv);

	Serial ser;
	ser.autoconnect(args.baudrate);

	AudioWriter audiowriter(args.dartt_address, args.dartt_index, ser);
	audiowriter.play(args.filename);

    printf("\nDone!\n");
    return 0;
}