#ifndef INC_CONFIG_H
#define INC_CONFIG_H

//ISR runtime measurement using testpin PA12
#define DEBUG_ISR if(1)  // 0: no pin toggling  1: enable pin toggling. Increases runtime and maybe ADC noise.

#define MAX_CHANNELS 4

//# 4chn; Oversampling = 32
// cycles per conversion = 15
//fs = 42.5MHz / (15*4*32) = 22.1 kHz
//F_UART = 22.1k * 4chn * 16bit = 1416.6 kbps

//#define FRAMES 22   // buffer_size = 2byte * 4 * 22 =  176byte; UART update rate: 22.135kHz/22  = 1 kHz
//#define FRAMES 221 // buffer_size = 2byte * 4 * 221 = 1768byte; UART update rate: 22.135kHz/221 = 100 Hz -> 10ms
#define FRAMES 443   // buffer_size = 2byte * 4 * 443 = 3544byte; UART update rate: 22.135kHz/443 =  50 Hz -> 20ms


#endif /* INC_CONFIG_H */
