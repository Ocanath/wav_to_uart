from wiretime import get_interframe_delay_us, get_throughput_fps, wire_time

serial_overhead = 5


nbytes_from_measurement = 9
t_us_from_measurement = 123.141


pc_ift_us = get_interframe_delay_us(nbytes_from_measurement, t_us_from_measurement, 921600, 1, 0)
print(f"Interframe delay is {pc_ift_us} microseconds")
actual_est_throughput = get_throughput_fps(32, 2, serial_overhead, 921600, 1, 0, pc_ift_us)
print(f"From PC, based on {nbytes_from_measurement} bytes at {t_us_from_measurement} us wire time, your throughput will be:\n    {actual_est_throughput} Hz")




tus_from_STM32 = 335
nbytes_from_STM32 = 31
stm32_ift_us = get_interframe_delay_us(nbytes_from_STM32, tus_from_STM32, 921600, 1, 0)
print(f"On stm32, interframe delay is {stm32_ift_us}")
actual_est_throughput = get_throughput_fps(32, 2, serial_overhead, 921600, 1, 0, stm32_ift_us)
print(f"From STM32, you can expect {actual_est_throughput} Hz output freq")


## dartt dashboard input streaming calculations

stm32_response_time = 18.444e-6		#seconds
pc_read_response_time = 230.70e-6

nb_serial_readrequest = 2+2+1+2+2
nb_max_read_reply = 1*4+(5+2)	#subject to change
nframes = (nb_max_read_reply - (serial_overhead + 2)) / 4
print(f"Number of frames read back per read request is {nframes}")
read_request_wiretime = wire_time(nb_serial_readrequest, 921600, 1, 0, pc_ift_us)
print(f"RRWT {read_request_wiretime*1e6}")
read_request_response_wiretime = wire_time(nb_max_read_reply, 921600, 1, 0, 0)
print(f"RRRWT {read_request_response_wiretime*1e6}")

exchange_time_total = (read_request_response_wiretime + stm32_response_time + read_request_response_wiretime + pc_read_response_time)
nostream_hz = nframes/exchange_time_total
print(f"Total time {exchange_time_total*1e6} microseconds")
print(f"est dartt non-streaming throughput: {nostream_hz} Hz")


## estimated audio throughput given response timing
bd_audiostreaming = 921600
nb_max_read_reply = 71	#subject to change
nframes = (nb_max_read_reply - (serial_overhead + 2)) / 2	#2 byte frame size, 2 byte adtl overhead from index
print(f"Nframes is {nframes}")

pc_audioblock_transmission_time = wire_time(nb_max_read_reply, bd_audiostreaming, 1, 0, pc_ift_us)
nb_audio_status = 1+2+2+2+4
stm32_status_response = wire_time(nb_audio_status, bd_audiostreaming, 1, 0, 0)
total_time = pc_audioblock_transmission_time + stm32_response_time + read_request_wiretime + stm32_response_time + stm32_status_response + pc_read_response_time
audio_streaming_throughput = nframes/total_time
print(f"Estimated PC streaming throughput with status polling, best case scenario given perfectly timed read request is {audio_streaming_throughput} Hz")

audio_streaming_nopoll_throughput = nframes/(pc_audioblock_transmission_time)
print(f"If status polling is (somehow) eliminated, PC throughput could theoretically grow to {audio_streaming_nopoll_throughput} Hz")
