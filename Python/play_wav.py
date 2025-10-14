import argparse
from wav_serial import play_wav
from serial_helper import autoconnect_serial

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Read and process WAV files for UART transmission')
    parser.add_argument('filename', type=str, help='Path to the WAV file to read')
    args = parser.parse_args()
    try:
        slist = autoconnect_serial(921600)
        if(len(slist) != 0):
            ser = slist[0]
    except:
        raise RuntimeWarning("Failed to connect to a serial port")
    
    play_wav(ser, args.filename)
    
