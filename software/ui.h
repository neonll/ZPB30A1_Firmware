#ifndef _UI_H_
#define _UI_H_
#include <stdbool.h>
#include <stdint.h>

void ui_init();
void ui_timer();

void showMenu();


extern volatile int8_t encoder_val;
extern volatile bool encoder_pressed;
extern volatile bool run_pressed;
extern uint8_t brightness[];
extern bool option_changed;
extern volatile bool redraw;
#endif
