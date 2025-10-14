import argparse
import numpy as np
from scipy.io.wavfile import read
import scipy as sp
from cobs import encode
from serial_helper import autoconnect_serial
import struct




def main():
    parser = argparse.ArgumentParser(description='Read and process WAV files for UART transmission')
    parser.add_argument('filename', type=str, help='Path to the WAV file to read')

    args = parser.parse_args()

    # Read the WAV file
    sample_rate, data = read(args.filename)

    print(f"Sample rate: {sample_rate} Hz")
    print(f"Data shape: {data.shape}")
    print(f"Data dtype: {data.dtype}")

    if(len(data.shape) != 1):
        raise RuntimeError("Audio must be mono-encoded")


    try:
        slist = autoconnect_serial(921600)
        if(len(slist) != 0):
            ser = slist[0]
    except:
        raise RuntimeWarning("Failed to connect to a serial port")

    buffered_pkt = bytearray([])
    for dval in data:
        pkt_bytes = struct.pack('<h', dval)
        pkt = bytearray(encode(pkt_bytes))
        buffered_pkt.extend(pkt)
        # if(len(buffered_pkt) > 48):
        #     try:
        #         ser.write(buffered_pkt)
        #     except:
        #         print(f'No serial port. latest dval = {dval}, encoded = {pkt.hex()}, pkt = {buffered_pkt.hex()}')
        #     buffered_pkt = bytearray([])
    ser.write(buffered_pkt)


if __name__ == '__main__':

    main()



