#ifndef WIRETIME_H
#define WIRETIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t wire_time_us(int nbytes, int baudrate, int nstopbits, int nparitybits, int32_t interframe_delay_us);
float get_throughput_fps(int nframes, int framesize, int nbytes_overhead, int baudrate, int nstopbits, int nparitybits, int32_t interframe_delay_us);
int32_t get_interframe_delay_us(int nbytes, int32_t total_wiretime_us, int baudrate, int nstopbits, int nparitybits);
int32_t bytes_per_us(int32_t us, int baudrate, int nstopbits, int nparitybits, int32_t interframe_delay_us);

#ifdef __cplusplus
}
#endif

#endif /* WIRETIME_H */
