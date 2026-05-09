#include "wiretime.h"
#include "unity.h"
#include <stdint.h>

void setUp(void) {}
void tearDown(void) {}

/* --- wire_time_us ---
 * nbits_per_byte = 8 + 1(start) + nstopbits + nparitybits
 * bit_time_us    = nbytes * nbits_per_byte * 1000000 / baudrate  (integer truncation)
 * gap_time_us    = nbytes * interframe_delay_us
 */

void test_wire_time_us_basic(void)
{
    /* 10 bytes * 10 bits * 1e6 / 921600 = 108.5 -> 108 us */
    TEST_ASSERT_EQUAL_INT32(108, wire_time_us(10, 921600, 1, 0, 0));
}

void test_wire_time_us_interframe(void)
{
    /* bit_time=108 us + gap=10*10=100 us -> 208 us */
    TEST_ASSERT_EQUAL_INT32(208, wire_time_us(10, 921600, 1, 0, 10));
}

void test_wire_time_us_clean_baudrate(void)
{
    /* 10 bytes * 10 bits * 1e6 / 1000000 = 100 us exactly */
    TEST_ASSERT_EQUAL_INT32(100, wire_time_us(10, 1000000, 1, 0, 0));
}

void test_wire_time_us_two_stop_bits(void)
{
    /* nbits_per_byte=11; 10*11*1e6/1000000 = 110 us */
    TEST_ASSERT_EQUAL_INT32(110, wire_time_us(10, 1000000, 2, 0, 0));
}

void test_wire_time_us_parity_bit(void)
{
    /* nbits_per_byte=11 (parity); 10*11*1e6/1000000 = 110 us */
    TEST_ASSERT_EQUAL_INT32(110, wire_time_us(10, 1000000, 1, 1, 0));
}

/* --- bytes_per_us ---
 * nbytes = (us * baudrate) / (nbits_per_byte * 1000000 + interframe_delay_us * baudrate)
 */

void test_bytes_per_us_clean_baudrate(void)
{
    /* 100*1e6 / (10*1e6) = 10 bytes exactly */
    TEST_ASSERT_EQUAL_INT32(10, bytes_per_us(100, 1000000, 1, 0, 0));
}

void test_bytes_per_us_921600(void)
{
    /* 1000*921600 / (10*1000000) = 921600000/10000000 = 92 bytes */
    TEST_ASSERT_EQUAL_INT32(92, bytes_per_us(1000, 921600, 1, 0, 0));
}

void test_bytes_per_us_with_interframe(void)
{
    /* num=1000*921600=921600000; den=10*1e6+10*921600=19216000; 921600000/19216000=47 */
    TEST_ASSERT_EQUAL_INT32(47, bytes_per_us(1000, 921600, 1, 0, 10));
}

/* --- get_throughput_fps ---
 * UNITY_EXCLUDE_FLOAT is set, so compare as int32_t with a 1 fps tolerance.
 */

void test_throughput_fps_clean_baudrate(void)
{
    /* nbytes=1; t_us=10; fps=1e6/10=100000 */
    float fps = get_throughput_fps(1, 1, 0, 1000000, 1, 0, 0);
    TEST_ASSERT_INT_WITHIN(1, 100000, (int32_t)fps);
}

void test_throughput_fps_typical(void)
{
    /* nbytes=100*2+5=205; t_us=205*10*1e6/921600=2224 us; fps=100*1e6/2224=44964 */
    float fps = get_throughput_fps(100, 2, 5, 921600, 1, 0, 0);
    TEST_ASSERT_INT_WITHIN(1, 44964, (int32_t)fps);
}

void test_throughput_fps_with_interframe(void)
{
    /* nbytes=10; bit_us=108; gap=10*5=50; t_us=158; fps=1e6/158=6329 */
    float fps = get_throughput_fps(1, 10, 0, 921600, 1, 0, 5);
    TEST_ASSERT_INT_WITHIN(1, 6329, (int32_t)fps);
}
