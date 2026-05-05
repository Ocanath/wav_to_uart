from wiretime import get_interframe_delay_us, get_throughput_fps

nbytes_from_measurement = 9
t_us_from_measurement = 122.63600001460873


ift_us = get_interframe_delay_us(nbytes_from_measurement, t_us_from_measurement, 921600, 1, 0)
print(f"Interframe delay is {ift_us} microseconds")
actual_est_throughput = get_throughput_fps(32, 2, 5, 921600, 1, 0, ift_us)
print(f"Based on {nbytes_from_measurement} bytes at {t_us_from_measurement} us wire time, your throughput will be:\n    {actual_est_throughput} Hz")


