ADC Clock fadc=170MHz/4=42.5MHz

Sampling time:
2,5cyc + 12,5cyc = 15cyc

Channels = 4

# Oversampling = 16
fs = 42.5MHz / (15*4*16) = 44.27 kHz
F_UART = 44.27k * 4chn * 16bit = 2833.3 kbps

# Oversampling = 32
fs = 42.5MHz / (15*4*32) = 22.1 kHz
F_UART = 22.1k * 4chn * 16bit = 1416.6 kbps


115.2 kbps * 16 = 1843.2 kbps brutto -> netto 8b/10 : 1474.56 kbps

2000 kbps brutto -> netto 8b/10 : 1600 kbps



```
picocom -b 115200 --imap lfcrlf /dev/ttyACM1
picocom -b 1843200 --imap lfcrlf /dev/ttyACM1
picocom -b 2000000 --imap lfcrlf /dev/ttyACM1
```
