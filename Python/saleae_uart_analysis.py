"""
Analyze a Saleae Logic 2 UART decoder CSV export.

Expected CSV format (Logic 2 async serial export):
    Time [s],Value,Parity Error,Framing Error
    4.493217602000000,0x03,,
    ...

Usage:
    python saleae_uart_analysis.py decoded_bytes.csv --baudrate 921600
    python saleae_uart_analysis.py decoded_bytes.csv --baudrate 921600 --frame-gap-us 40
"""

import argparse
import numpy as np
import pandas as pd
import sys


def load_csv(path):
    df = pd.read_csv(path, usecols=[0, 1], names=["time_s", "value"], header=0)
    # Drop any rows where time is not parseable (error annotations, etc.)
    df["time_s"] = pd.to_numeric(df["time_s"], errors="coerce")
    df.dropna(subset=["time_s"], inplace=True)
    return df["time_s"].to_numpy(dtype=np.float64)


def byte_duration_s(baudrate, nstopbits, nparitybits):
    return (8 + 1 + nstopbits + nparitybits) / baudrate


def analyze(timestamps, baudrate, nstopbits, nparitybits, frame_gap_us):
    nbytes = len(timestamps)
    byte_s = byte_duration_s(baudrate, nstopbits, nparitybits)

    # Gap between start of byte[n+1] and end of byte[n]
    gaps_us = (np.diff(timestamps) - byte_s) * 1e6

    total_wire_us = (timestamps[-1] - timestamps[0] + byte_s) * 1e6

    print("=== Capture summary ===")
    print(f"  Total bytes      : {nbytes:,}")
    print(f"  Total wire time  : {total_wire_us/1e6:.4f} s  ({total_wire_us:,.0f} us)")
    print(f"  Baudrate         : {baudrate}")
    print(f"  Byte duration    : {byte_s*1e6:.3f} us")

    print("\n=== Inter-byte gap stats ===")
    print(f"  Mean  : {np.mean(gaps_us):.3f} us")
    print(f"  Std   : {np.std(gaps_us):.3f} us")
    print(f"  Min   : {np.min(gaps_us):.3f} us")
    print(f"  Max   : {np.max(gaps_us):.3f} us")
    neg = np.sum(gaps_us < -0.5)  # >0.5us negative indicates a framing/baud mismatch
    if neg:
        print(f"  WARNING: {neg} negative gaps — check baudrate / stop bit settings")

    if frame_gap_us is not None:
        _frame_stats(timestamps, gaps_us, frame_gap_us, byte_s)


def _frame_stats(timestamps, gaps_us, frame_gap_us, byte_s):
    boundary_mask = gaps_us > frame_gap_us
    boundary_idx = np.where(boundary_mask)[0]  # index into gaps_us / byte[n]

    n_frames = len(boundary_idx) + 1
    inter_frame_gaps = gaps_us[boundary_idx]

    # Frame byte counts
    starts = np.concatenate([[0], boundary_idx + 1])
    ends   = np.concatenate([boundary_idx + 1, [len(timestamps)]])
    frame_sizes = ends - starts

    # Per-frame wire time (start of first byte to end of last byte in frame)
    frame_wire_us = np.array([
        (timestamps[e - 1] - timestamps[s] + byte_s) * 1e6
        for s, e in zip(starts, ends)
    ])

    print(f"\n=== Frame detection (threshold = {frame_gap_us} us) ===")
    print(f"  Frames detected  : {n_frames:,}")
    print(f"  Bytes/frame — mean {np.mean(frame_sizes):.1f}  "
          f"min {np.min(frame_sizes)}  max {np.max(frame_sizes)}")
    print(f"  Frame wire time  — mean {np.mean(frame_wire_us):.1f} us  "
          f"min {np.min(frame_wire_us):.1f} us  max {np.max(frame_wire_us):.1f} us")
    print(f"\n=== Inter-frame gap stats (the big gaps) ===")
    print(f"  Mean  : {np.mean(inter_frame_gaps):.3f} us")
    print(f"  Std   : {np.std(inter_frame_gaps):.3f} us")
    print(f"  Min   : {np.min(inter_frame_gaps):.3f} us")
    print(f"  Max   : {np.max(inter_frame_gaps):.3f} us")


def main():
    parser = argparse.ArgumentParser(description="Analyze Saleae Logic 2 UART CSV export")
    parser.add_argument("csv", help="Path to exported CSV (Time [s], Value, ...)")
    parser.add_argument("--baudrate",      type=int,   default=921600)
    parser.add_argument("--stopbits",      type=int,   default=1)
    parser.add_argument("--paritybits",    type=int,   default=0)
    parser.add_argument("--frame-gap-us",  type=float, default=None,
                        help="Gap threshold in us used to split byte stream into frames. "
                             "If omitted, only per-byte stats are reported.")
    args = parser.parse_args()

    print(f"Loading {args.csv} ...", flush=True)
    timestamps = load_csv(args.csv)
    print(f"Loaded {len(timestamps):,} bytes.\n")

    analyze(timestamps, args.baudrate, args.stopbits, args.paritybits, args.frame_gap_us)


if __name__ == "__main__":
    main()
