#ifndef _UI_H_
#define _UI_H_
#include <stdbool.h>
#include <stdint.h>

void ui_timer();


void showMenu();
void showText(char text[], uint8_t display);
void showNumber(uint16_t num, uint8_t dot, uint8_t display);

extern volatile int8_t encoder_val;
extern volatile bool encoder_pressed;
extern volatile bool run_pressed;
extern uint8_t brightness[];
extern bool option_changed;
extern volatile bool redraw;
#endif
