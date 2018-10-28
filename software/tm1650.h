#ifndef _TM1650_H_
#define _TM1650_H_
#include <stdint.h>

extern uint8_t digits[];
extern uint8_t chars[];

#define CHAR_OFFSET 48
#define LED_V       0x01
#define LED_AH      0x02
#define LED_WH      0x04
#define LED_A       0x08
#define LED_RUN     0x10
#define LED_HIGH    0x20
#define LED_LOW     0x40

typedef enum {
    BRIGHTNESS_BRIGHT = 5,
    BRIGHTNESS_DIM = 3
} brightness_t;

typedef enum {
    DP_TOP,
    DP_BOT
} display_t;

void disp_char(uint8_t position, uint8_t c, uint8_t dot, uint8_t pin);
void disp_brightness(uint8_t brightness, uint8_t pin);

#endif
