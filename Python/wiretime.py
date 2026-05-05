
"""
Function to compute the time 'on the wire' that a multi-byte UART frame will occupy as a function of all relevant settings.
Inputs: 
	nbytes: number of total bytes in the frame
	baudrate: serial baudrate. default 921600
	nstopbits: basic uart setting defining framesize. default 1. Is most commonly 1 or 2. 
	nparitybits: basic uart setting for parity bit. Default 0. Can be 0 or 1. Not input filtered so be careful
	interframe_delay_us: delay between frames in microseconds. 
		Note: On a real system, you may want to compute this as an average across multiple frames. Use the helper 
		function get_interframe_delay_us to determine this from an average measurement

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


"""
Computes the average delay time between bytes for a real measurement of N bytes. Measure the time
elapsed on the wire to transmit N bytes (in MICROSECONDS), and input N and the wiretime, as well as the UART settings.

Outputs the average interbyte time in microseconds
"""
def get_interframe_delay_us(nbytes, total_wiretime_us, baudrate=921600, nstopbits=1, nparitybits=0):
	theoretical_wiretime_us = wire_time(nbytes, baudrate, nstopbits, nparitybits, 0)
	tdif = total_wiretime_us - theoretical_wiretime_us*1e6
	if(tdif < 0):	
		print("WARNING: listed parameters produce an impossible result. Check input settings for correctness")
		# return 0
	return tdif/nbytes



"""
Determine how many bytes must be dispatched in one frame in order to occupy a specific time interval.
NOTE: Input time interval is MICROSECONDS, not seconds.
"""
def bytes_per_us(us, baudrate=921600, nstopbits=1, nparitybits=0, interframe_delay_us=0):
	nstartbits = 1	#always 1 for UART
	nbits_per_byte = 8+nstartbits+nstopbits+nparitybits
	nbytes =  (us*1e-6) / (((nbits_per_byte)/baudrate) + interframe_delay_us*1e-6)	#total wire time in seconds
	return nbytes


