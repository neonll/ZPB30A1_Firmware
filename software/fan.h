#ifndef _FAN_H_
#define _FAN_H_
#include <stdint.h>

extern uint16_t temperature;	// 0,1°C

void fan_init();
void fan_timer();
#endif
