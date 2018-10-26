#ifndef _LOAD_H_
#define _LOAD_H_
#include "bool.h"
extern bool cutoff_active;
extern uint16_t cutoff_voltage;
void tempFan();

typedef enum {
	ERROR_NONE,
	ERROR_UVP,  // Undervoltage protection
	ERROR_OVP,  // Overvoltage protection
	ERROR_OLP,  // Overload protection/warning
	ERROR_OTP,  // Over temperature protection
	ERROR_PWR,  // Insufficient power source
} error_t;
extern error_t error;
extern uint16_t  voltage; //0,01V
extern uint16_t set_current; //mA
extern volatile uint32_t mAmpere_seconds;	//mAs
extern volatile uint32_t mWatt_seconds;	//mWs
extern bool running;
#endif
