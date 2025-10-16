from scipy.io.wavfile import read
from cobs import encode
import struct




def play_wav(ser, filename):

    # Read the WAV file
    sample_rate, data = read(filename)

    if(len(data.shape) != 1):
        # raise RuntimeError("Audio must be mono-encoded")
        return False

    buffered_pkt = bytearray([])
    for dval in data:
        pkt_bytes = struct.pack('<h', dval)
        pkt = bytearray(encode(pkt_bytes))
        buffered_pkt.extend(pkt)
    try:
        ser.write(buffered_pkt)
    except Exception as e:
        # print(buffered_pkt.hex())
        raise RuntimeError(f"Serial Write failed:{type(e).__name__}:{e}")
    return True





