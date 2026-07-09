#!/usr/bin/env python3

import h5py
import numpy as np
import matplotlib.pyplot as plt
import sys

MAX_SAMPLES = int(200e9)
TRIG_LEVEL = [None, 'a0.71', None, None]  # r=rising, f=falling, a=absolute_value
#TRIG_LEVEL = [None, None, None, None]  # r=rising, f=falling, a=absolute_value
TRIG_FILTER = 2  # number of samples matching the trigger condition to filter outliers

PLOT_MODE = 'COMBINED'
#PLOT_MODE = 'STACKED'
#PLOT_MODE = 'SEPERATE_WINDOWS'

filename = sys.argv[1]

trig_channels = list()

with h5py.File(filename, "r") as f:
    dset = f["samples"]

    data = dset[:]

    channel_names = [x.decode() if isinstance(x, bytes) else x
                     for x in dset.attrs["channel_names"]]

    units = [x.decode() if isinstance(x, bytes) else x
             for x in dset.attrs["units"]]

    gains = np.asarray(dset.attrs["gains"], dtype=float)
    offsets = np.asarray(dset.attrs["offsets"], dtype=float)

    num_samples = len(data[channel_names[0]])
    print('num_samples =', num_samples)

    trig_level_raw = [None, None, None, None]
    trig_level_low_raw = [0, 0, 0, 0]
    for idx, lvl in enumerate(TRIG_LEVEL):
        if lvl != None:
            trig_level_raw[idx] = int((float(lvl[1:]) / gains[idx]) - offsets[idx])
            trig_level_low_raw[idx] = int((-float(lvl[1:]) / gains[idx]) - offsets[idx])  # for absolute_value trigger
            trig_channels.append(idx)
    print('TRIG_LEVEL', TRIG_LEVEL)
    print('trig_level_raw', trig_level_raw)
    print('trig_level_low_raw', trig_level_low_raw)

    samplerate = dset.attrs.get("samplerate", None)
    print('Samplerate =', samplerate, 'Hz')

n_channels = len(channel_names)


if PLOT_MODE == 'COMBINED':
    fig, ax1 = plt.subplots()
    ax2 = ax1.twinx()
    ax2._get_lines.prop_cycler = ax1._get_lines.prop_cycler

    start_idx = -1
    progress_percent = 0
    trig_samples = 0

    for ch_idx, name in enumerate(channel_names):

        if TRIG_LEVEL[ch_idx] != None:
            print(f"Searching trigger condition for chn{ch_idx+1}...")
            for idx, sample in enumerate(data[name]):
                if    ( (TRIG_LEVEL[ch_idx][0] == 'r' or TRIG_LEVEL[ch_idx][0] == 'a') and sample > trig_level_raw[ch_idx] ) \
                   or ( TRIG_LEVEL[ch_idx][0] == 'f' and sample < trig_level_raw[ch_idx] ) \
                   or ( TRIG_LEVEL[ch_idx][0] == 'a' and sample < trig_level_low_raw[ch_idx] ):

                    trig_samples += 1
                    if trig_samples >= TRIG_FILTER:
                        start_idx = max(0, idx-50)
                        print(f"found at idx {idx}.")
                        break
                else:
                    trig_samples = 0

                if num_samples > 10e6:
                    if idx/num_samples > progress_percent/100:
                        print(progress_percent, '% samples analysed.')
                        progress_percent += 10

        else:
            continue

        if start_idx != -1:
            break
        else:
            print(f"Trigger condition not found.")
            start_idx = 0


    for ch_idx, name in enumerate(channel_names):

        raw = data[name].astype(np.float64)[start_idx:start_idx+MAX_SAMPLES]

        x = np.arange(len(raw))
        scaled = (raw + offsets[ch_idx]) * gains[ch_idx]

        if ch_idx == 0:
            ax1.plot(x, scaled, label=name)
            ax1.set_xlabel(f"Sample ({samplerate} Hz)")
            ax1.set_ylabel(f"[{units[ch_idx]}]")

        else:
            ax2.plot(x, scaled, label=name)

    ax1.legend(loc='upper left')
    ax2.legend(loc='upper right')

    plt.title(filename)
    plt.grid(True)


elif PLOT_MODE == 'STACKED':
    fig, axes = plt.subplots(n_channels, 1, sharex=True)

    if n_channels == 1:
        axes = [axes]

    for ch_idx, name in enumerate(channel_names):
        raw = data[name].astype(np.float64)[:MAX_SAMPLES]
        scaled = (raw + offsets[ch_idx]) * gains[ch_idx]

        axes[ch_idx].plot(scaled)
        axes[ch_idx].set_ylabel(f"{name}\n[{units[ch_idx]}]")
        axes[ch_idx].grid(True)

        axes[-1].set_xlabel(f"Sample ({samplerate} Hz)")

elif PLOT_MODE == 'SEPERATE_WINDOWS':
    for ch_idx, name in enumerate(channel_names):

        raw = data[name].astype(np.float64)[:MAX_SAMPLES]

        scaled = (raw + offsets[ch_idx]) * gains[ch_idx]

        plt.figure()
        plt.plot(scaled)

        plt.xlabel(f"Sample ({samplerate} Hz)")

        plt.ylabel(f"{name}\n[{units[ch_idx]}]")
        plt.title(name)
        plt.grid(True)

plt.show()
