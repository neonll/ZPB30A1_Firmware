#ifndef _SETTINGS_H_
#define _SETTINGS_H_
#include "bool.h"
typedef enum {
	MODE_CC,
	MODE_CW,
	MODE_CR,
	MODE_CV,
} sink_mode_t;
extern sink_mode_t set_mode;
extern uint16_t set_values[4]; // CC/CW/CR/CV
extern bool beeper_enabled;
extern bool logging;
extern bool cutoff_active;
extern uint16_t cutoff_voltage;
void settings_init();
#endif
