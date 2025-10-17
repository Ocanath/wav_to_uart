from scipy.io.wavfile import read
from cobs import encode
import struct
import serial
import threading
import queue
from serial_helper import autoconnect_serial


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


class SerialPlayer:
    def __init__(self, port=None, baudrate=921600):
        self.port = port
        self.baudrate = baudrate
        self.command_queue = queue.Queue()
        self.abort_flag = threading.Event()
        self.worker_thread = None
        self.running = False
        self.ser = None

    def start(self):
        """Start the worker thread"""
        self.running = True
        self.worker_thread = threading.Thread(target=self._worker, daemon=True)
        # Initialize serial connection
        if(self.port != None):
            self.ser = serial.Serial(self.port, baudrate=self.baudrate, write_timeout=None)
            print(f"Connected to {self.port}")
        else:
            try:
                slist = autoconnect_serial(921600)
                if(len(slist) != 0):
                    self.ser = slist[0]
            except:
                raise RuntimeWarning("Failed to connect to a serial port")

        self.worker_thread.start()

    def stop(self):
        """Stop the worker thread"""
        self.running = False
        self.command_queue.put(None)  # Signal worker to exit
        if self.worker_thread:
            self.worker_thread.join()

    def play(self, filename):
        """Queue a file to play"""
        self.command_queue.put(('play', filename))

    def abort(self):
        """Abort current playback"""
        self.abort_flag.set()

    def _worker(self):
        """Worker thread that handles serial communication"""
        try:

            while self.running:
                try:
                    # Wait for command with timeout to allow checking running flag
                    cmd = self.command_queue.get(timeout=0.1)

                    if cmd is None:
                        break

                    command, *args = cmd

                    if command == 'play':
                        filename = args[0]
                        print(f"Playing: {filename}")
                        self.abort_flag.clear()  # Clear any previous abort

                        try:
                            result = play_wav(self.ser, filename, chunk_size=64, abort_flag=self.abort_flag)
                            if result:
                                print(f"Finished: {filename}")
                            else:
                                print(f"Aborted: {filename}")
                        except Exception as e:
                            print(f"Error playing {filename}: {e}")

                except queue.Empty:
                    continue

        except serial.SerialException as e:
            print(f"Serial error: {e}")
        finally:
            if self.ser and self.ser.is_open:
                self.ser.close()
                print("Serial port closed")
