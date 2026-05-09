from wiretime import get_interframe_delay_us, get_throughput_fps, wire_time




## Original cobs encoded single frame
bd_audiostreaming = 921600

original_encodingfreq = 17631	#fps
adjust_nframes = 1000
original_wiretime_for_1000_frames_us = adjust_nframes/original_encodingfreq *1e6
original_nbytes_per_frame = 4

adjusted_pc_ift_us = get_interframe_delay_us(adjust_nframes*original_nbytes_per_frame, original_wiretime_for_1000_frames_us, bd_audiostreaming, 1, 0)
pc_ift_us = adjusted_pc_ift_us
print(f"Based on og audio, adjusted interframe delay is {adjusted_pc_ift_us}")
original_frame_wire_time = wire_time(original_nbytes_per_frame*1, bd_audiostreaming, 1, 0, adjusted_pc_ift_us)
og_samplingfreq = 1/original_frame_wire_time
print(f"Original method with 50% overhead yielded {og_samplingfreq} samples per second audio sampling frequency")


serial_overhead = 7	#1 byte address, 2 bytes index, 2 bytes crc, 2 bytes cobs
nbytes_from_measurement = 9
t_us_from_measurement = 123.141


tus_from_STM32 = 335
nbytes_from_STM32 = 31
stm32_ift_us = get_interframe_delay_us(nbytes_from_STM32, tus_from_STM32, bd_audiostreaming, 1, 0)
print(f"    On stm32, interframe delay is {stm32_ift_us}")
if(stm32_ift_us < 0):
	print("    This small negative value is essentially measurement error from the logic analyzer. Treat STM32 interbyte delay as equal to zero")
actual_est_throughput = get_throughput_fps(32, 2, serial_overhead, bd_audiostreaming, 1, 0, stm32_ift_us)
print(f"From STM32, you can expect {actual_est_throughput} Hz output freq")


## dartt dashboard input streaming calculations

stm32_response_time = 18.444e-6		#seconds
pc_read_response_time = 230.70e-6

nb_serial_readrequest = 2+2+1+2+2
nb_max_read_reply = 1*4+(5+2)	#subject to change
nframes = (nb_max_read_reply - serial_overhead) / 4
print(f"Number of frames read back per read request is {nframes}")
read_request_wiretime = wire_time(nb_serial_readrequest, bd_audiostreaming, 1, 0, pc_ift_us)
print(f"RRWT {read_request_wiretime*1e6}")
read_request_response_wiretime = wire_time(nb_max_read_reply, bd_audiostreaming, 1, 0, 0)
print(f"RRRWT {read_request_response_wiretime*1e6}")

exchange_time_total = (read_request_response_wiretime + stm32_response_time + read_request_response_wiretime + pc_read_response_time)
nostream_hz = nframes/exchange_time_total
print(f"Total time {exchange_time_total*1e6} microseconds")
print(f"est dartt non-streaming throughput: {nostream_hz} Hz")


## Recalc pc ift
ntotalbytes = 5132552 - 246
totaltime = 72.663615874
pc_ift_us = get_interframe_delay_us(ntotalbytes, totaltime*1e6, 921600, 1, 0)
print(f"New pc ift us is: {pc_ift_us}")

## estimated audio throughput given response timing
bd_audiostreaming = 460800
serial_overhead = 7
nb_audioblocksize = 64+serial_overhead	#subject to change
nframes = (nb_audioblocksize - serial_overhead) / 2	#2 byte frame size, 2 byte adtl overhead from index
print(f"Nframes is {nframes}")

pc_audioblock_transmission_time_us = wire_time(nb_audioblocksize, bd_audiostreaming, 1, 0, pc_ift_us)*1e6
print(f"Frame transmission time is {pc_audioblock_transmission_time_us} microseconds")
print(f"Individual frame wire time (retransmission_us must be {pc_audioblock_transmission_time_us/nframes}")	#you have to emit 32 frames in one audioblock transmission time
rounded_retransmissionus = int(pc_audioblock_transmission_time_us/nframes)
print(f"Rounded retransmission us: {rounded_retransmissionus}")
actual_block_playbacktime = rounded_retransmissionus*nframes
tailmsg = ""
if(actual_block_playbacktime < pc_audioblock_transmission_time_us):
	tailmsg =  f", beating the block transmission by {pc_audioblock_transmission_time_us - actual_block_playbacktime}"
else:
	tailmsg = f", lagging behind the next block transmission by {actual_block_playbacktime - pc_audioblock_transmission_time_us}"
print(f"So the motor should theoretically actually finish block playback in {actual_block_playbacktime} us" + tailmsg)
audio_streaming_nopoll_throughput = nframes/(pc_audioblock_transmission_time_us/1e6)
print(f"If running nosync eliminated, you must encode audio file at {audio_streaming_nopoll_throughput} fps")

