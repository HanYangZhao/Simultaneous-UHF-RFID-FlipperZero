#!/usr/bin/env python3
"""Capture Flipper Zero CLI log output for a fixed duration.

Usage: python3 capture_log.py [seconds]
Opens the Flipper serial CLI, issues the `log` command, and prints
everything received for the given number of seconds (default 20).
"""
import sys
import time
import glob

try:
    import serial
except ImportError:
    sys.exit("pyserial not installed. Run: pip install pyserial")

DURATION = int(sys.argv[1]) if len(sys.argv) > 1 else 20

ports = glob.glob("/dev/cu.usbmodemflip_*")
if not ports:
    sys.exit("No Flipper serial device found (/dev/cu.usbmodemflip_*)")
port = ports[0]

with serial.Serial(port, 115200, timeout=0.1) as ser:
    time.sleep(0.2)
    ser.reset_input_buffer()
    # Enter CLI, start log streaming
    ser.write(b"\r\n")
    time.sleep(0.2)
    ser.write(b"log trace\r\n")
    print(f"# Streaming log from {port} for {DURATION}s. Do the scan now...", flush=True)
    deadline = time.time() + DURATION
    while time.time() < deadline:
        data = ser.read(4096)
        if data:
            sys.stdout.write(data.decode("utf-8", errors="replace"))
            sys.stdout.flush()
    # Ctrl-C to stop log, return to prompt
    ser.write(b"\x03")
    time.sleep(0.1)
print("\n# --- capture complete ---", flush=True)
