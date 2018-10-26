#ifndef _FAN_H_
#define _FAN_H_
#include "bool.h"
extern volatile bool calc_fan;
void fan_init();

// TODO: Clean interface
void tempFan();
void setFan();
extern uint16_t temperature;	// 0,1°C
#endif
