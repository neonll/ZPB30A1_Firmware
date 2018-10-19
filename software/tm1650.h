#ifndef _TM1650_H_
#define _TM1650_H_
#include <stdint.h>

//TODO: Move this to .c and create proper functions
extern uint8_t digits[];
extern uint8_t chars[];

#define CHAR_OFFSET 54
#define LED_V       0x01
#define LED_AH      0x02
#define LED_WH      0x04
#define LED_A       0x08
#define LED_RUN     0x10
#define LED_HIGH    0x20
#define LED_LOW     0x40

void disp_write(uint8_t addr, uint8_t data, uint8_t pin);
void setBrightness(uint8_t brightness, uint8_t pin);

#endif
