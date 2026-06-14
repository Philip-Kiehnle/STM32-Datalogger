# STM32-Datalogger

An STM32G4 Nucleo-32 board (NUCLEO-G431KB) is used to build this datalogger.  
Four ADC channels are continuously sampled and streamed via UART-USB to a host PC. The host PC can configure the decimation factor to be able reduce the maximum samplerate of 22.1 ksps by a factor of 2, 4 or 8. Hardware averaging is used to increase the signal quality. The python script [readUART_Fast_Monitor.py](pythonMonitorTool/readUART_Fast_Monitor.py) is used to store the raw 16bit ADC data in a binary or HDF5 file.

ADC2 channel connection:  
Channel 1: Differential PA0 PA1  
Channel 2: Single-Ended PA6  
Channel 3: Single-Ended PA7  
Channel 4: Single-Ended PA4  

## Howto

### Install python packages
All components require the python venv setup:
```bash
python3 -m venv .venv
source .venv/bin/activate
#pip install pyserial numpy h5py matplotlib
#pip freeze > requirements.txt
pip install -r requirements.txt
```

### Allow USB Serial Access 
```
sudo adduser $USER dialout
```
New login is required.

### Run
For testing, the [readUART_Fast_Monitor.py](pythonMonitorTool/readUART_Fast_Monitor.py) is used in direct plot mode. It also writes a binary file for plotting the data again.
```
python3 readUART_Fast_Monitor.py /dev/ttyACM0
```

If longer measurements should be acquired, the HDF5 file format is selected in the config section. The application does not plot anything directly but continues sampling until the application is stopped. The data is directly written to the HDF5 file.
Metadata like the scaling and samplerate is also stored in the HDF5 file. The [plot.py](pythonMonitorTool/plot.py) can be used to scale the data to SI units and plot the curves.

```
python3 plot.py measure_latest.hdf5
```

The FAT file system has a file size limit of 4GB, so at 4chn 22ksps the approximated storage time is:  
4Gb / (22ksps × 4chn × 2byte) = ~6 hours
With prescaling to float datatype, the duration would decrease to:  
4Gb / (22ksps × 4chn × 4byte) = ~3 hours

## ADC Setup

STM32 clock: 170MHz  
ADC clock: fadc = 170MHz/4 = 42.5MHz

Sampling time:  
2,5cyc + 12,5cyc = 15cyc

### 2 Channels; Oversampling factor = 32 (decimation factor = 1)
fs = 42.5MHz / (15×2×32) = 44.27 kHz  
F_UART = 44.27k × 2chn × 16bit = 1416.6 kbps

### 4 Channels; Oversampling factor = 32 (decimation factor = 1)
fs = 42.5MHz / (15×4×32) = 22.1 kHz  
F_UART = 22.1k × 4chn × 16bit = 1416.6 kbps

UART has start and stop bits, assume 8b/10 netto rate.  
2000 kbps brutto -> netto 8b/10 : 1600 kbps > 1416.6 kbps and should be sufficient for full rate streaming of four channels.



# Debug
```
picocom -b 2000000 --imap lfcrlf /dev/ttyACM0
```


# Notes
AMC3330 noise without extra filter: 31780 to 31800  
AMC3330 noise with extra 330pF C filter between P and N: 31670 to 31680  
LEM noise without extra filter: 48510 to 48550  
LEM noise with extra 1nF C filter and microcontroller pin pulldown enabled: 48495 to 48520
