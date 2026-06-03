# STM32-Datalogger

An STM32G4 Nucleo-32 board (NUCLEO-G431KB) is used to build this datalogger.  
Four ADC channels are continuously sampled and streamed via UART-USB to a host PC. The host PC can configure the decimation factor to be able reduce the maximum samplerate of 22.1 ksps by a factor of 2, 4 or 8. Hardware averaging is used to increase the signal quality. The python script [readUART_Fast_Monitor.py](pythonMonitorTool/readUART_Fast_Monitor.py) is used to store the raw 16bit ADC data in a binary or HDF5 file.


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
