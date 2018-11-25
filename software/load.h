#ifndef _LOAD_H_
#define _LOAD_H_
#include <stdbool.h>
#include <stdint.h>
#include "settings.h"

//NOTE: Keep this enum in sync with the messages in ui_error_handler()!
typedef enum {
	ERROR_NONE,
	ERROR_POLARITY, // input polarity reversed
	ERROR_OVERVOLTAGE, // input voltage to high
	ERROR_OVERLOAD,
	ERROR_OVERTEMPERATURE, // heat sink temperature to high
	ERROR_POWER_SUPPLY, // supply voltage to low
	ERROR_TIMER_OVERFLOW, // timer tick lost
	ERROR_INTERNAL, // other internal error
} error_t;
extern error_t error;

typedef enum {
	CAL_NONE,
	CAL_CURRENT,
} calibration_t;

extern bool load_active;
extern uint16_t current_setpoint;
extern calibration_t calibration_step;
extern uint16_t calibration_value;

/* Current setpoint after all constraints are taken into account. */
extern uint16_t actual_current_setpoint;
extern uint32_t mAmpere_seconds;
extern uint32_t mWatt_seconds;


void load_init();
void load_timer();
void load_enable();
void load_disable();

#endif
