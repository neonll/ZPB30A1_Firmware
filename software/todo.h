#ifndef _TODO_H_
#define _TODO_H_

#include <stdint.h>
typedef uint8_t bool;

typedef enum {
	MODE_CC,
	MODE_CW,
	MODE_CR,
	MODE_CV,
} sink_mode_t;
extern sink_mode_t set_mode;
extern uint16_t set_values[4]; // CC/CW/CR/CV
extern bool cutoff_active ;
extern uint16_t cutoff_voltage;
extern bool beeper_on;
void tempFan();

#endif
