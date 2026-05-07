#!/usr/bin/python3

import ctypes
import serial
#import signal
import matplotlib.pyplot as plt
import sys
import struct
import time
import numpy as np


seconds = 2
samplerate = 22.1e3

d = np.dtype([
('v_dc', 'u2'),
('i_dc', 'u2'),
('i_ac1', 'i2'),
('i_ac2', 'i2'),
])

# b int8
# B uint8
# h int16
# H uint16
# i uint32
# I uint32
# f float32
fast_monitor_vars_t = "HHhh"

struct_size = struct.calcsize(fast_monitor_vars_t)
FAST_MON_FRAMES = int(seconds*samplerate)
vars = np.zeros(FAST_MON_FRAMES, dtype=d)
x = np.arange(FAST_MON_FRAMES)
FAST_MON_BYTES = FAST_MON_FRAMES*struct_size
BYTES_PER_FRAME = 443


def ser_read_exact(ser, size):
    bytesAvail = ser.inWaiting()
    while bytesAvail < size:
        bytesAvail = ser.inWaiting()
    packet = ser.read(size)
    return packet


def stop_fast_monitor_mode(ser):
    ser.write('d'.encode())  # stop active monitor mode
    ser.readall()


def start_fast_monitor_mode(ser):
    if ser.read(1):
        stop_fast_monitor_mode(ser)

    ser.write('f'.encode())  # start fast monitor mode
    magic_bytes = b'Fast monitor TRIG\n'
    line = ser.readline()
    print(line)
    if line == magic_bytes:
        print("started monitoring")


def plot(ax, name, gain = 1.0):

    #y = np.clip(gain*vars[name], -5000, 5000)  # clip UART errors
    y = gain*vars[name]
    ax.plot(x, y, label=name)


def main():

    for name in d.names:
        print("{0:20} {1}".format(name,  d.fields[name]))

    if (d.itemsize != struct_size) :
        print('struct missmatch: d.itemsize = ', d.itemsize, '!=', struct_size, 'struct_size')
        sys.exit()


    if len(sys.argv) != 2:
        print('Usage', sys.argv[0], '/dev/ttyUSB')
        sys.exit()

    #signal.signal(signal.SIGINT, signal_handler)

    if sys.argv[1].find('/dev/tty') != -1:
        ser = serial.Serial(sys.argv[1], 2000000, timeout=1)
        start_fast_monitor_mode(ser)

        bytes_to_read = FAST_MON_BYTES
        packet = b''  # Initialize packet as an empty bytes object
        start_time = time.time()
        seq_time = start_time

        bytes_read = 0

        while bytes_to_read > 0:
            r_len = min(bytes_to_read, BYTES_PER_FRAME)
            packet += ser_read_exact(ser, r_len)
            bytes_read += r_len
            bytes_to_read -= r_len

            # Print sample rate and duration every 0.5 seconds
            current_time = time.time()
            elapsed_total = current_time - start_time
            elapsed_seq = current_time - seq_time
            if elapsed_seq >= 0.5:
                sample_rate = bytes_read / elapsed_seq  # bytes per second
                print(f"Sample rate: {sample_rate:.2f} bytes/s, Duration: {elapsed_total:.2f}s")
                seq_time = current_time  # Reset the timer
                bytes_read = 0  # Reset the bytes counter


        stop_fast_monitor_mode(ser)
        ser.close()

        with open('monitor_vars_fast.bin', 'wb') as file:
            file.write(packet)

        for i in range(FAST_MON_FRAMES):
            vars[i] = struct.unpack_from(fast_monitor_vars_t, packet[i*struct_size:(i+1)*struct_size])

    else:
        with open(sys.argv[1], 'rb') as file:
            for i in range(FAST_MON_FRAMES):
                vars[i] = struct.unpack_from(fast_monitor_vars_t, file.read(struct_size))

    #value = np.clip(0.1*vars['v_dc'], -5000, 5000)
    #print(f'v_dc min={min(value):.3f} avg={sum(value)/FAST_MON_FRAMES:.3f} max={max(value):.3f} ')
    #value = np.clip(0.01*vars['i_dc'], -5000, 5000)
    #print(f'i_dc  min={min(value):.3f} avg={sum(value)/FAST_MON_FRAMES:.3f} max={max(value):.3f} ')
    #value = np.clip(0.01*vars['i_ac1'], -5000, 5000)
    #print(f'i_ac1  min={min(value):.3f} avg={sum(value)/FAST_MON_FRAMES:.3f} max={max(value):.3f} ')


    fig, ax1 = plt.subplots()
    ax1.grid(axis='x')

    plot(ax1, 'v_dc', 1.0)
    plot(ax1, 'i_dc', 1.0)

    ax2 = ax1.twinx()
    ax2._get_lines.prop_cycler = ax1._get_lines.prop_cycler

    plot(ax2, 'i_ac1', 1.0)
    plot(ax2, 'i_ac2', 1.0)

    ax1.legend(loc='upper left')
    ax2.legend(loc='upper right')

    #leg = interactive_legend()

    #plt.savefig('timeplot.png', dpi=300)
    plt.show()


if __name__=='__main__':
    main()

