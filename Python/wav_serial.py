from scipy.io.wavfile import read
from cobs import encode
import struct




def play_wav(ser, filename, chunk_size=64, abort_flag=None):

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

    # Write in chunks to avoid timeout and allow abort
    offset = 0
    while offset < len(buffered_pkt):
        # Check abort flag
        if abort_flag is not None and abort_flag.is_set():
            return False

        # Get chunk
        chunk = buffered_pkt[offset:offset + chunk_size]

        try:
            ser.write(chunk)
        except Exception as e:
            raise RuntimeError(f"Serial Write failed:{type(e).__name__}:{e}")

        offset += chunk_size

    return True





