import argparse
import numpy as np
from scipy.io.wavfile import read
import scipy as sp
from cobs import encode

import matplotlib.pyplot as plt





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

    fig,ax = plt.subplots()
    t = np.linspace(0,len(data), len(data))
    ax.plot(t, data)
    plt.show()


if __name__ == '__main__':
    main()



