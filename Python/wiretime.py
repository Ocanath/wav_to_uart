
"""
Function to compute the time 'on the wire' that a multi-byte UART frame will occupy as a function of all relevant settings.
Inputs: 
	nbytes: number of total bytes in the frame
	baudrate: serial baudrate. default 921600
	nstopbits: basic uart setting defining framesize. default 1. Is most commonly 1 or 2. 
	nparitybits: basic uart setting for parity bit. Default 0. Can be 0 or 1. Not input filtered so be careful
	interframe_delay_us: delay between frames in microseconds. 
		Note: On a real system, you may want to compute this as an average across multiple frames.
			1. Measure the wire time to send N bytes
			2. Subtract the wire_tim for an interframe delay = 0
			3. 

"""
def wire_time(nbytes, baudrate=921600, nstopbits=1, nparitybits=0, interframe_delay_us=0):
	# nbytes = 64

	nstartbits = 1	#always 1 for UART
	nbits_per_byte = 8+nstartbits+nstopbits+nparitybits
	t_total_sec = ((nbytes*nbits_per_byte)/baudrate) + nbytes*interframe_delay_us*1e-6	#total wire time in seconds
	return t_total_sec


#5 bytes for dartt mode 0 (serial) + cobs, 2 byte framesize for 16byte audio frames
def get_throughput_fps(nframes, framesize=2, nbytes_overhead=5, baudrate=921600, nstopbits=1, nparitybits=0, interframe_delay_us=0):
	nbytes = nframes*framesize+nbytes_overhead
	tsec = wire_time(nbytes, baudrate, nstopbits, nparitybits, interframe_delay_us)
	return nframes/tsec

