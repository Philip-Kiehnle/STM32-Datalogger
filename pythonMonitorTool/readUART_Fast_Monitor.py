#!/usr/bin/python3

import ctypes
import serial
#import signal
import matplotlib.pyplot as plt
import sys
import struct
import time
import numpy as np

from dataclasses import dataclass

@dataclass
class scale:
    unit: str
    offset: float
    gain: float


seconds = 1.0
channels = 2  # 2 or 4 channels supported
max_samplerate_2chn = 44.2708e3
max_samplerate_4chn = 22.1354e3
decimation_factor = 1  # supported decimation factors: 1,2,4,8

max_samplerate = max_samplerate_2chn if (channels==2) else max_samplerate_4chn
samplerate = max_samplerate/decimation_factor

# sensor calibration and scale to SI units
chnX_scale = [
              scale(unit = 'V',
                    offset = -0.5 * 2**16 + 1091,
                    gain = 0.99 * 1.25 * 0.23/16),  # scale to volt

              scale(unit = 'A',
                    offset = -2.5/3.3 * 2**16 + 1129,
                    gain = 0.989 * 3.3/(2**16) / 0.025),  # scale to ampere 25mV/A

              scale(unit = 'raw',
                    offset = 0.0,
                    gain = 1.0),

              scale(unit = 'raw',
                    offset = 0.0,
                    gain = 1.0)
            ]

d = np.dtype([  #i2 for int16
('v', 'u2'),
('i', 'u2'),
('chn3', 'u2'),
('chn4', 'u2'),
])

# b int8
# B uint8
# h int16
# H uint16
# i uint32
# I uint32
# f float32
fast_monitor_vars_t = "HHHH"

if channels == 2:
    d = np.dtype(d.descr[:2])
    fast_monitor_vars_t = fast_monitor_vars_t[:2]

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
    ser.write('x'.encode())  # stop active monitor mode
    ser.readall()


def start_fast_monitor_mode(ser):
    if ser.read(1):
        stop_fast_monitor_mode(ser)

    chn_mode_char = 't' if (channels==2) else 'f'
    ser.write(chn_mode_char.encode())  # set channel mode: 2(two) or 4(four) 
    ser.readline()
    ser.write('d'.encode())  # set decimation factor
    ser.readline()
    ser.write(str(decimation_factor).encode())
    ser.readline()

    ser.write('s'.encode())  # start fast monitor mode
    magic_bytes = b'Monitoring stream START\n'
    line = ser.readline()
    print(line)
    if line == magic_bytes:
        print("started monitoring")
        return True
    return False


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
        print('Usage', sys.argv[0], '/dev/ttyACMx')
        sys.exit()

    #signal.signal(signal.SIGINT, signal_handler)

    if sys.argv[1].find('/dev/tty') != -1:
        ser = serial.Serial(sys.argv[1], 2000000, timeout=1)

        if start_fast_monitor_mode(ser):

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

    i = 0
    scaled_samples = np.empty([4, FAST_MON_FRAMES])
    for field_name in d.names:
        scaled_samples[i] = (vars[field_name]+chnX_scale[i].offset) * chnX_scale[i].gain
        i +=1
     
    # for calibration
    #print(f"avg chn1: {vars[d.names[0]].mean()}")
    #print(f"avg chn2: {vars[d.names[1]].mean()}")
    #print(f"avg chn2: {scaled_samples[1].mean()}")

    fig, ax1 = plt.subplots()
    ax1.grid(axis='x')

    #plot(ax1, 'v', 1.0)
    ax1.plot(x, scaled_samples[0], label=d.names[0])


    ax2 = ax1.twinx()
    ax2._get_lines.prop_cycler = ax1._get_lines.prop_cycler

    #plot(ax2, 'i', 1.0)
    ax2.plot(x, scaled_samples[1], label=d.names[1])

    ax1.legend(loc='upper left')
    ax2.legend(loc='upper right')

    #leg = interactive_legend()

    #plt.savefig('timeplot.png', dpi=300)
    plt.show()


if __name__=='__main__':
    main()

