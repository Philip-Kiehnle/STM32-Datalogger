#!/usr/bin/env python3

import h5py
import numpy as np
import matplotlib.pyplot as plt
import sys

MAX_SAMPLES = int(200e9)
#TRIG_LEVEL = [None, -0.5, None, None] # chn2 Ampere
TRIG_LEVEL = [None, None, None, None]

PLOT_MODE = 'COMBINED'
#PLOT_MODE = 'STACKED'
#PLOT_MODE = 'SEPERATE_WINDOWS'

filename = sys.argv[1]

with h5py.File(filename, "r") as f:
    dset = f["samples"]

    data = dset[:]

    channel_names = [x.decode() if isinstance(x, bytes) else x
                     for x in dset.attrs["channel_names"]]

    units = [x.decode() if isinstance(x, bytes) else x
             for x in dset.attrs["units"]]

    gains = np.asarray(dset.attrs["gains"], dtype=float)
    offsets = np.asarray(dset.attrs["offsets"], dtype=float)

    samplerate = dset.attrs.get("samplerate", None)

n_channels = len(channel_names)


if PLOT_MODE == 'COMBINED':
    fig, ax1 = plt.subplots()
    ax2 = ax1.twinx()
    ax2._get_lines.prop_cycler = ax1._get_lines.prop_cycler

    start_idx = 0

    for ch_idx, name in enumerate(channel_names):

        if TRIG_LEVEL[ch_idx] != None:
            print(f"Searching trigger condition for chn{ch_idx+1}...")
            raw = data[name].astype(np.float64)
            scaled = (raw + offsets[ch_idx]) * gains[ch_idx]
            for idx, sample in enumerate(scaled):
                if sample < TRIG_LEVEL[ch_idx]:
                    start_idx = max(0, idx-50)
                    print(f"found at idx {idx}.")
                    break
        else:
            continue

        if start_idx != 0:
            break
        else:
            print(f"Trigger condition not found.")


    for ch_idx, name in enumerate(channel_names):

        raw = data[name].astype(np.float64)[start_idx:+MAX_SAMPLES]

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
