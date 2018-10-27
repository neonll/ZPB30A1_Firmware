#ifndef _SETTINGS_H_
#define _SETTINGS_H_
#include "bool.h"


typedef enum {
	MODE_CC,
	MODE_CW,
	MODE_CR,
	MODE_CV,
	NUM_MODES
} sink_mode_t;

typedef struct {
	sink_mode_t mode;
	uint16_t setpoints[NUM_MODES]; // CC/CW/CR/CV
	bool beeper_enabled;
	bool cutoff_enabled;
	uint16_t cutoff_voltage;
} settings_t;

extern settings_t settings;

void settings_init();
void settings_update();
#endif
