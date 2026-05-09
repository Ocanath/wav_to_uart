#include "wiretime.h"
#include <stdint.h>
#include <stdio.h>

int32_t wire_time_us(int nbytes, int baudrate, int nstopbits, int nparitybits, int32_t interframe_delay_us)
{
    int nstartbits = 1;
    int nbits_per_byte = 8 + nstartbits + nstopbits + nparitybits;
    int64_t bit_time_us = (int64_t)nbytes * nbits_per_byte * 1000000 / baudrate;
    int64_t gap_time_us = (int64_t)nbytes * interframe_delay_us;
    return (int32_t)(bit_time_us + gap_time_us);
}

float get_throughput_fps(int nframes, int framesize, int nbytes_overhead, int baudrate, int nstopbits, int nparitybits, int32_t interframe_delay_us)
{
    int nbytes = nframes * framesize + nbytes_overhead;
    int32_t t_us = wire_time_us(nbytes, baudrate, nstopbits, nparitybits, interframe_delay_us);
    return (float)nframes*1000000.f/((float)t_us);
}

int32_t get_interframe_delay_us(int nbytes, int32_t total_wiretime_us, int baudrate, int nstopbits, int nparitybits)
{
    int32_t theoretical_us = wire_time_us(nbytes, baudrate, nstopbits, nparitybits, 0);
    int32_t tdif = total_wiretime_us - theoretical_us;
    if (tdif < 0)
        fprintf(stderr, "WARNING: listed parameters produce an impossible result. Check input settings for correctness\n");
    return tdif / nbytes;
}

int32_t bytes_per_us(int32_t us, int baudrate, int nstopbits, int nparitybits, int32_t interframe_delay_us)
{
    int nstartbits = 1;
    int nbits_per_byte = 8 + nstartbits + nstopbits + nparitybits;
    int64_t numerator   = (int64_t)us * baudrate;
    int64_t denominator = (int64_t)nbits_per_byte * 1000000 + (int64_t)interframe_delay_us * baudrate;
    return (int32_t)(numerator / denominator);
}
